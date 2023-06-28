#pragma once

#include <vector>
#include <string>
#include <filesystem>
#include "nrnpython.h"

namespace neuron::morphology {

    class MorphIOWrapper;
    
    // MorphIO API
    std::vector<std::string> morphio_read(std::filesystem::path const& path);
    void morphio_read(PyObject* pyObj, MorphIOWrapper& morph);
    

    // Morphology constructs
    std::string type2name(int type_id);
    std::string mksubset(int type_id, int freq, const std::string& type_name);
    std::string name(int type_id, int index);

}