#pragma once

#include <vector>
#include <string>
#include <filesystem>

namespace neuron::morphology {
    
    // MorphIO API
    std::vector<std::string> morphio_read(std::filesystem::path const& path);
    

    // Morphology constructs
    std::string type2name(int type_id);
    std::string mksubset(int type_id, int freq, const std::string& type_name);
    std::string name(int type_id, int index);

}