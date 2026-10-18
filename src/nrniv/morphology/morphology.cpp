#include "morphology.hpp"
#include "oc_ansi.h"
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>
#include <morphio/morphology.h>
#include <morphio/section.h>
#include <morphio/soma.h>
#include <unordered_map>
#include <fmt/core.h>
#include "morphio_wrapper/morphio_wrapper.hpp"
#include "nrnpy.h"

namespace neuron::morphology {
std::vector<std::string> morphio_load(std::filesystem::path const& path) {
    return MorphIOWrapper{path}.morph_as_hoc();
}


std::string type2name(int type_id) {
    static const std::unordered_map<int, std::string> type2name_dict = {{1, "soma"},
                                                                        {2, "axon"},
                                                                        {3, "dend"},
                                                                        {4, "apic"}};
    if (const auto it = type2name_dict.find(type_id); it != type2name_dict.end()) {
        return it->second;
    }
    return (type_id < 0) ? fmt::format("minus_{}", -type_id) : fmt::format("dend_{}", type_id);
}

std::string mksubset(int type_id, [[maybe_unused]] int freq, const std::string& type_name) {
    static const std::unordered_map<int, std::string> mksubset_dict = {{1, "somatic"},
                                                                       {2, "axonal"},
                                                                       {3, "basal"},
                                                                       {4, "apical"}};
    std::string tstr;
    if (const auto it = mksubset_dict.find(type_id); it != mksubset_dict.end()) {
        tstr = it->second;
    } else if (type_id < 0) {
        tstr = fmt::format("minus_{}set", -type_id);
    } else {
        tstr = fmt::format("dendritic_{}", type_id);
    }
    return fmt::format("forsec \"{}\" {}.append", type_name, tstr);
}

std::string name(int type_id, int index) {
    return fmt::format("{}[{}]", type2name(type_id), index);
}

void morphio_load(PyObject* pyObj, const MorphIOWrapper& morph) {
    if (!neuron::python::methods.po2ho) {
        hoc_execerror("Python support is disabled", nullptr);
    }
    // Call nrn_po2ho on the PyObject
    Object* cell_obj = neuron::python::methods.po2ho(pyObj);

    // Check that the PyObject is a Cell object
    if (!cell_obj) {
        hoc_execerror("morphio_load requires a Cell object", nullptr);
    }

    auto result = morph.morph_as_hoc();
    // execute the hoc code in the cell_obj context
    auto err = 0.;
    for (const auto& s: result) {
        err = hoc_obj_run(s.c_str(), cell_obj);
        if (err)
            break;
    }
    if (err) {
        hoc_execerror("morphio_load failed", nullptr);
    }
}
}  // namespace neuron::morphology


void morphio_load() {
    if (!ifarg(1)) {
        hoc_execerror("morphio_load requires a Cell object", nullptr);
    }
    // get the Cell object and check that it is a Cell object
    auto cell_obj = *hoc_objgetarg(1);
    if (!cell_obj) {
        hoc_execerror("morphio_load requires a Cell object", nullptr);
    }

    if (!ifarg(2)) {
        hoc_execerror("morphio_load requires a path argument", nullptr);
    }
    // get the morphology path
    std::filesystem::path path(hoc_gargstr(2));
    auto result = neuron::morphology::morphio_load(path);

    // temporary: also dump to file where the morphology is located
    // i.e. /path/to/morphology.h5 -> /path/to/morphology.hoc
    if (ifarg(3) && *hoc_getarg(3) == 1) {
        // get the path stem and append .hoc
        std::string write_file = path.stem().string() + ".hoc";
        std::filesystem::path write_path(path.parent_path() / write_file);
        auto f = std::fopen(write_path.c_str(), "w+");
        if (!f) {
            hoc_execerror("Cannot open file for writing:", write_path.c_str());
        }
        for (const auto& s: result) {
            fmt::print(f, "{}\n", s);
        }
        std::fclose(f);
    }

    // execute the hoc code in the cell_obj context
    auto err = 0.;
    for (const auto& s: result) {
        err = hoc_obj_run(s.c_str(), cell_obj);
        if (err != 0)
            break;
    }
    hoc_retpushx(err ? 0. : 1.);
}
