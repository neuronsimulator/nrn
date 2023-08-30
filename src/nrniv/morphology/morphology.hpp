#pragma once

#include <vector>
#include <string>
#include <filesystem>
#include "nrnpy.h"

namespace neuron::morphology {

class MorphIOWrapper;

// MorphIO API
std::vector<std::string> morphio_load(std::filesystem::path const& path);
void morphio_load(PyObject* pyObj, MorphIOWrapper& morph);


// Morphology constructs
std::string type2name(int type_id);
std::string mksubset(int type_id, int freq, const std::string& type_name);
std::string name(int type_id, int index);

}  // namespace neuron::morphology