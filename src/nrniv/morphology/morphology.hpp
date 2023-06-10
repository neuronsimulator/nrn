#pragma once

#include <vector>
#include <string>
#include <filesystem>

namespace neuron::morphology {
    std::vector<std::string> morphio_read(std::filesystem::path const& path);
    std::string type2name(int type_id);
    std::string mksubset(int type_id, int freq, const std::string& type_name);
    std::string name(int type_id, int index);
}
