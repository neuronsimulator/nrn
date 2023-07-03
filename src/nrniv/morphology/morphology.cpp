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
#include "morphio_wrapper/morphio_wrapper.hpp"
#include "nrnpython.h"

namespace neuron::morphology {
std::vector<std::string> morphio_read(std::filesystem::path const& path) {
    return MorphIOWrapper{path}.morph_as_hoc();
}


std::string type2name(int type_id) {
    static const std::unordered_map<int, std::string> type2name_dict = {{1, "soma"},
                                                                        {2, "axon"},
                                                                        {3, "dend"},
                                                                        {4, "apic"}};
    auto it = type2name_dict.find(type_id);
    if (it != type2name_dict.end()) {
        return it->second;
    } else {
        return (type_id < 0) ? "minus_" + std::to_string(-type_id)
                             : "dend_" + std::to_string(type_id);
    }
}

std::string mksubset(int type_id, int freq, const std::string& type_name) {
    static const std::unordered_map<int, std::string> mksubset_dict = {{1, "somatic"},
                                                                       {2, "axonal"},
                                                                       {3, "basal"},
                                                                       {4, "apical"}};
    auto it = mksubset_dict.find(type_id);
    std::string tstr = (it != mksubset_dict.end())
                           ? it->second
                           : ((type_id < 0) ? "minus_" + std::to_string(-type_id) + "set"
                                            : "dendritic_" + std::to_string(type_id));
    return "forsec \"" + type_name + "\" " + tstr + ".append";
}

std::string name(int type_id, int index) {
    return type2name(type_id) + "[" + std::to_string(index) + "]";
}

void morphio_read(PyObject* pyObj, MorphIOWrapper& morph) {
    // Call nrn_po2ho on the PyObject
    Object* cell_obj = nrnpy_po2ho(pyObj);

    // Check that the PyObject is a Cell object
    if (!cell_obj) {
        hoc_execerror("morphio_read requires a Cell object", nullptr);
    }

    auto result = morph.morph_as_hoc();
    // execute the hoc code in the cell_obj context
    auto err = 0.;
    for_each(result.begin(), result.end(), [&err, &cell_obj](std::string const& s) {
        err = hoc_obj_run(s.c_str(), cell_obj);
    });
    if (err) {
        hoc_execerror("morphio_read failed", nullptr);
    }
}
}  // namespace neuron::morphology


void morphio_read() {
    if (!ifarg(1)) {
        hoc_execerror("morphio_read requires a Cell object", nullptr);
    }
    // get the Cell object and check that it is a Cell object
    auto cell_obj = *hoc_objgetarg(1);

    if (!ifarg(2)) {
        hoc_execerror("morphio_read requires a path argument", nullptr);
    }
    // get the morphology path
    std::filesystem::path path(hoc_gargstr(2));
    auto result = neuron::morphology::morphio_read(path);

    // temporary: also dump to file where the morphology is located
    // i.e. /path/to/morphology.h5 -> /path/to/morphology.hoc
    if (ifarg(3) && *hoc_getarg(3) == 1) {
        // get the path stem and append .hoc
        std::string write_file = path.stem().c_str();
        write_file.append(".hoc");
        std::filesystem::path write_path(path.parent_path() / write_file);
        auto f = std::fopen(write_path.c_str(), "w+");
        for_each(result.begin(), result.end(), [&f](std::string const& s) {
            // print to file
            std::fprintf(f, "%s\n", s.c_str());
        });
        std::fclose(f);
    }

    // execute the hoc code in the cell_obj context
    auto err = 0.;
    for_each(result.begin(), result.end(), [&err, &cell_obj](std::string const& s) {
        err = hoc_obj_run(s.c_str(), cell_obj);
    });
    hoc_retpushx(err ? 0. : 1.);
}
