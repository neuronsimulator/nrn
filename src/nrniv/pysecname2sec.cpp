#include <../../nrnconf.h>

#include <stdio.h>
#include <hocparse.h>

#include <parse.hpp>

#include <nrnoc2iv.h>

#include "nrnsymdiritem.h"
#include "utils/enumerate.h"

#include <string>
#include <map>

static bool activated = false;  // true on first use of hoc __pysec.

enum CorStype { CELLTYPE, SECTYPE, OVERLOADCOUNT, NONETYPE };
typedef std::pair<CorStype, void*> CellorSec;
typedef std::map<const std::string, CellorSec> Name2CellorSec;
static Name2CellorSec n2cs;

#define hoc_acterror(a, b) printf("%s %s\n", a, b)

static void activate() {
#if USE_PYTHON
    if (!activated) {
        // printf("first activation\n");
        activated = true;
        hoc_Item* qsec;
        // ForAllSections(sec)
        ITERATE(qsec, section_list) {
            Section* sec = hocSEC(qsec);
            if (sec->prop && sec->prop->dparam[PROP_PY_INDEX].get<void*>()) {
                nrnpy_pysecname2sec_add(sec);
            }
        }
    }
#endif
}

Section* nrnpy_pysecname2sec(const char* name) {  // could be Cell part or Sec
    activate();
    std::string n(name);
    if (nrn_parsing_pysec_ == (void*) 1) {
        Name2CellorSec::iterator search = n2cs.find(n);
        if (search != n2cs.end()) {
            if (search->second.first == SECTYPE) {
                nrn_parsing_pysec_ = NULL; /* done */
                return (Section*) search->second.second;
            } else if (search->second.first == CELLTYPE) {
                nrn_parsing_pysec_ = search->second.second;
            } else if (search->second.first == OVERLOADCOUNT) {
                nrn_parsing_pysec_ = NULL;
                hoc_acterror(
                    name,
                    " is an overloaded first part name for multiple sections created in python");
            }
        } else {
            nrn_parsing_pysec_ = NULL;
            hoc_acterror(name, " is not a valid first part name for section created in python");
        }
    } else {
        Name2CellorSec* n2s = (Name2CellorSec*) nrn_parsing_pysec_;
        Name2CellorSec::iterator search = n2s->find(n);
        if (search != n2s->end()) {
            if (search->second.first == OVERLOADCOUNT) {
                nrn_parsing_pysec_ = NULL;
                hoc_acterror(
                    name,
                    " is an overloaded second part name for multiple sections created in python");
            }
            nrn_parsing_pysec_ = NULL; /* done */
            assert(search->second.first == SECTYPE);
            return (Section*) search->second.second;
        } else {
            nrn_parsing_pysec_ = NULL;
            hoc_acterror(name, " is not a valid last part name for section created in python");
        }
    }
    return NULL;
}

// Want to add a Section to a cell but one may already be there or
// already be overloaded.
static void n2s_add(Name2CellorSec& n2s, std::string n, Section* sec) {
    Name2CellorSec::iterator search = n2s.find(n);
    if (search == n2s.end()) {
        n2s[n] = CellorSec(SECTYPE, sec);
    } else {
        CellorSec& cs = search->second;
        if (cs.first == SECTYPE) {
            cs.first = OVERLOADCOUNT;
            cs.second = (void*) 2;
        } else if (cs.first == OVERLOADCOUNT) {
            cs.second = (void*) ((size_t) cs.second + 1);
        }
    }
}

// Want to add a Section to top level but name may already exist. If so
// already a Section -- overload 2
// already overloaded section -- overload increase by 1
// already a cell -- no longer usable, becomes NONETYPE
// already NONETYPE -- nothing to do
static void n2cs_add_sec(Name2CellorSec& n2cs, std::string sname, Section* sec) {
    // printf("n2cs_add_sec %s section=%p\n", sname.c_str(), sec);
    Name2CellorSec::iterator search = n2cs.find(sname);
    if (search == n2cs.end()) {
        n2cs[sname] = CellorSec(SECTYPE, sec);
    } else {
        CellorSec& cs = search->second;
        if (cs.first == CELLTYPE) {  // conflict so destroy
            Name2CellorSec* n2s = (Name2CellorSec*) cs.second;
            delete n2s;
            cs.first = NONETYPE;
            cs.second = NULL;
        } else if (cs.first == SECTYPE) {  // overload
            cs.first = OVERLOADCOUNT;
            cs.second = (void*) 2;
        } else if (cs.first == OVERLOADCOUNT) {  // increase overload
            cs.second = (void*) ((size_t) cs.second + 1);
        }
    }
}

// Want to add a Cell to top level but name may already exist. If so
// already a Cell -- nothing to do
// already a overloaded section or section -- no longer usable, becomes NONETYPE
// already NONETYPE -- nothing to do
static Name2CellorSec* n2cs_add_cell(Name2CellorSec& n2cs, std::string cname) {
    Name2CellorSec::iterator search = n2cs.find(cname);
    Name2CellorSec* n2s = NULL;  // not usable if it stays NULL
    if (search == n2cs.end()) {  // not found add  cname, Name2Section*
        n2s = new Name2CellorSec();
        n2cs[cname] = CellorSec(CELLTYPE, n2s);
    } else {
        CellorSec& cs = search->second;
        if (cs.first == SECTYPE || cs.first == OVERLOADCOUNT) {  // conflict so destroy
            cs.first = NONETYPE;
            cs.second = NULL;
        } else if (cs.first == CELLTYPE) {  // exists, ready to use.
            n2s = (Name2CellorSec*) cs.second;
        }
    }
    return n2s;
}

void n2cs_add(Name2CellorSec& n2cs, std::string cname, std::string sname, Section* sec) {
    Name2CellorSec* n2s = n2cs_add_cell(n2cs, cname);
    // printf("n2cs_add %s %s n2s=%p section=%p\n", cname.c_str(), sname.c_str(), n2s, sec);
    if (n2s) {
        n2s_add(*n2s, sname, sec);
    }
}

void nrnpy_pysecname2sec_add(Section* sec) {
    if (!activated) {
        return;
    }
    std::string n(secname(sec));
    if (n.find("__nrnsec_0x", 0) == 0) {
        return;
    }
    if (n.find("<", 0) != n.npos) {
        return;
    }
    // printf("nrnpy_pysecname2sec_add %s\n", secname(sec));
    size_t dot = n.find('.', 1);
    if (dot != n.npos) {  // cell.sec
        std::string cname = n.substr(0, dot);
        std::string sname = n.substr(dot + 1);
        n2cs_add(n2cs, cname, sname, sec);
    } else {  // sec
        n2cs_add_sec(n2cs, n, sec);
    }
}

static bool decrement(CellorSec& cs) {
    cs.second = (void*) ((size_t) cs.second - 1);
    return cs.second ? false : true;
}

void nrnpy_pysecname2sec_remove(Section* sec) {
    if (!activated) {
        return;
    }
    std::string name(secname(sec));
    // printf("remove %s\n", name.c_str());
    if (name[0] == '<') {
        return;
    }

    size_t dot = name.find('.', 1);
    if (dot != name.npos) {  // cell.sec
        std::string cname = name.substr(0, dot);
        std::string sname = name.substr(dot + 1);
        Name2CellorSec::iterator it = n2cs.find(cname);
        assert(it != n2cs.end());
        // must be CELLTYPE or NONETYPE
        CellorSec& cs = it->second;
        if (cs.first == CELLTYPE) {
            Name2CellorSec* n2s = (Name2CellorSec*) cs.second;
            Name2CellorSec::iterator its = n2s->find(sname);
            assert(its != n2s->end());
            // must be SECTYPE or OVERLOADCOUNT
            CellorSec& css = its->second;
            if (css.first == SECTYPE) {
                n2s->erase(its);
                if (n2s->empty()) {
                    delete n2s;
                    n2cs.erase(it);
                }
            } else {
                assert(css.first == OVERLOADCOUNT);
                if (decrement(css)) {
                    n2s->erase(its);
                    if (n2s->empty()) {
                        delete n2s;
                        n2cs.erase(it);
                    }
                }
            }
        } else {
            assert(cs.first == NONETYPE);
        }
    } else {  // sec
        Name2CellorSec::iterator it = n2cs.find(name);
        assert(it != n2cs.end());
        // must be SECTYPE, NONETYPE, or OVERLOADCOUNT
        CellorSec& cs = it->second;
        if (cs.first == SECTYPE) {
            n2cs.erase(it);
        } else if (cs.first == OVERLOADCOUNT) {
            if (decrement(cs)) {
                n2cs.erase(it);
            }
        } else {
            assert(cs.first == NONETYPE);
        }
    }
#if 0
  if (n2cs.empty()) {
    printf("pysecname2sec is empty\n");
  }
#endif
}

void nrn_symdir_load_pysec(std::vector<SymbolItem*>& sl, void* v) {
    activate();
    if (!v) {
        // top level items are any of the four types
        for (auto&& [symbol, cs]: n2cs) {
            if (cs.first != NONETYPE && cs.first != OVERLOADCOUNT) {
                SymbolItem* si = new SymbolItem(symbol.c_str(), 0);
                si->pysec_type_ = cs.first == CELLTYPE ? PYSECOBJ : PYSECNAME;
                si->pysec_ = (Section*) cs.second;
                sl.push_back(si);
            }
        }
    } else {
        // in cell items are either OVERLOADCOUNT or SECTYPE
        for (auto&& [symbol, cs]: *static_cast<Name2CellorSec*>(v)) {
            if (cs.first == SECTYPE) {
                auto* si = new SymbolItem(symbol.c_str(), 0);
                si->pysec_type_ = PYSECNAME;
                si->pysec_ = (Section*) cs.second;
                sl.push_back(si);
            }
        }
    }
}
