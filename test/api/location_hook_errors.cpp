// A HOC point-process location or distance() argument that is an Object must be turned into a
// section and position by the Python location hook. Without a hook, or when the hook leaves the
// section NULL, that used to hit assert(0) and abort the process. It must be a HOC error.
#include "neuronapi.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>

extern "C" void modl_reg() {}

// make_pointprocess registers a mechanism for the rest of the process, and LeakSanitizer reports
// that registration as leaked at exit. Only an ASan build reads this hook.
extern "C" const char* __lsan_default_suppressions() {
    return "leak:make_pointprocess\n";
}

extern void (*nrnpy_o2loc_p_)(Object*, Section**, double*);
extern void (*nrnpy_o2loc2_p_)(Object*, Section**, double*);

static bool hook_saw_nil = false;

// Stands in for another provider's hook: it cannot make a section out of the Object.
static void leave_section_null(Object* o, Section**, double*) {
    if (!o) {
        hook_saw_nil = true;
    }
}

static bool check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
    }
    return condition;
}

static double hoc_value(const char* expr) {
    std::string stmt = std::string("loc_hook_value = ") + expr + "\n";
    nrn_hoc_call(stmt.c_str());
    return *nrn_symbol_dataptr(nrn_symbol("loc_hook_value"));
}

// get_loc() leaves the point process's section pushed.
static double get_loc(const char* name) {
    double x = hoc_value((std::string(name) + ".get_loc()").c_str());
    nrn_hoc_call("pop_section()\n");
    return x;
}

// Run a command that must fail with a HOC error containing `expected`.
static bool fails_with(const char* command, const char* expected) {
    std::fflush(stderr);
    FILE* const sink = std::tmpfile();
    int saved = dup(STDERR_FILENO);
    if (!sink || saved == -1 || dup2(fileno(sink), STDERR_FILENO) == -1) {
        std::perror("redirect stderr");
        std::exit(1);
    }
    int rc = nrn_hoc_call(command);
    std::fflush(stderr);
    dup2(saved, STDERR_FILENO);
    close(saved);
    std::string text;
    char buf[512];
    std::rewind(sink);
    size_t n;
    while ((n = std::fread(buf, 1, sizeof(buf), sink)) > 0) {
        text.append(buf, n);
    }
    std::fclose(sink);
    bool ok = rc != 0 && text.find(expected) != std::string::npos;
    if (!ok) {
        std::cerr << "FAIL: '" << command << "' rc=" << rc << " stderr='" << text << "'\n";
    }
    return ok;
}

// Every failing form, then proof that the interpreter still builds and measures correctly.
static bool exercise(const char* what) {
    static const char* seg = "not a Python nrn.Segment";
    static const char* seg2 =
        "not a Python nrn.Segment, rxd.node, or other with a segment property";
    bool ok = true;
    std::cerr << "case: " << what << '\n';
    ok &= fails_with("objref loc_ic\nloc_ic = new IClamp(loc_obj)\n", seg);
    ok &= fails_with("loc_ic = new IClamp(0.5)\nloc_ic.loc(loc_obj)\n", seg);
    ok &= fails_with("soma { loc_x = distance(loc_obj) }\n", seg2);
    ok &= fails_with("soma { loc_x = distance(0, loc_obj) }\n", seg2);
    ok &= fails_with("objref loc_hp\nsoma loc_hp = new HocPnt(new Vector(3))\n", seg);
    ok &= fails_with("soma loc_hp = new HocPnt(0.25)\nloc_hp.loc(new Vector(3))\n", seg);
    // A nil Object is an error too, and is not passed to a hook.
    ok &= fails_with("objref loc_nil, loc_ic3\nloc_ic3 = new IClamp(loc_nil)\n", seg);
    ok &= fails_with("loc_ic3 = new IClamp(0.5)\nloc_ic3.loc(loc_nil)\n", seg);
    ok &= fails_with("soma { loc_x = distance(loc_nil) }\n", seg2);
    ok &= fails_with("soma { loc_x = distance(loc_nil, 0.5) }\n", "Distance origin not valid");
    ok &= fails_with("soma { loc_x = distance(0, loc_nil) }\n", seg2);
    ok &= fails_with("objref loc_hp3\nsoma loc_hp3 = new HocPnt(loc_nilf())\n", seg);
    ok &= check(!hook_saw_nil, "a nil Object is not passed to the hook");
    // Failed constructions leave nothing behind: repeat them (leaks show under LeakSanitizer).
    ok &= check(
        nrn_hoc_call(
            "for i = 1, 1000 { execute1(\"objref loc_r\\nloc_r = new IClamp(loc_obj)\\n\", 0) "
            "execute1(\"objref loc_r\\nloc_r = new HocPnt(new Vector(3))\\n\", 0) }\n") == 0,
        "repeated failures");
    // The failed HocPnt construction must not leave last_created_pp_ob_ set.
    ok &= check(nrn_hoc_call("objref loc_hp2\nsoma loc_hp2 = new HocPnt(0.25)\n") == 0,
                "HocPnt constructs after a failed construction");
    ok &= check(get_loc("loc_hp2") == 0.25, "HocPnt located at 0.25");
    // Numbers still work, the section stack is balanced and the interpreter is usable.
    ok &= check(nrn_hoc_call("objref loc_ic2\nsoma loc_ic2 = new IClamp(0.75)\n") == 0,
                "IClamp(0.75) constructs");
    ok &= check(get_loc("loc_ic2") == 0.75, "IClamp located at 0.75");
    ok &= check(nrn_hoc_call("soma { distance(0, 0.25)  loc_x = distance(0.75) }\n") == 0,
                "distance with numbers");
    ok &= check(hoc_value("loc_x") == 100, "distance(0.75) from origin 0.25 is 100 um");
    return ok;
}

int main() {
    std::array<const char*, 4> args = {"location_hook_errors", "-nogui", "-nopython", nullptr};
    nrn_init(3, args.data());
    if (nrn_hoc_call("create soma\n"
                     "soma { L = 200  nseg = 2  diam = 10 }\n"
                     "objref loc_obj, loc_nilobj\n"
                     "obfunc loc_nilf() { return loc_nilobj }\n"
                     "loc_obj = new Vector(3)\n"
                     "begintemplate HocPnt\n"
                     "public a\n"
                     "proc init() { a = 3 }\n"
                     "endtemplate HocPnt\n"
                     "make_pointprocess(\"HocPnt\")\n"
                     "access soma\n") != 0) {
        std::cerr << "FAIL: setup\n";
        return 1;
    }
    bool ok = true;
    ok &= exercise("NEURON's hooks, not a Python object");
    nrnpy_o2loc_p_ = nullptr;
    nrnpy_o2loc2_p_ = nullptr;
    ok &= exercise("no hook");
    // A hook that cannot produce a section.
    nrnpy_o2loc_p_ = leave_section_null;
    nrnpy_o2loc2_p_ = leave_section_null;
    ok &= exercise("hook leaves the section NULL");
    return ok ? 0 : 1;
}
