#pragma once
#include <string>
#include <vector>
#include <memory>
#include <tuple>

namespace morphio {
    class Morphology;
    class Section;
}
namespace neuron::morphology {
    class MorphIOWrapper {
    public:
        explicit MorphIOWrapper(const std::string& filename);
        MorphIOWrapper(const MorphIOWrapper&) = delete;
        MorphIOWrapper& operator=(const MorphIOWrapper&) = delete;
        MorphIOWrapper(MorphIOWrapper&&) = default;
        MorphIOWrapper& operator=(MorphIOWrapper&&) = default;

        std::vector<std::string> morph_as_hoc() const;

    private:
        std::unique_ptr<morphio::Morphology> _morph;
        std::vector<morphio::Section> _sections;
        std::vector<std::tuple<int, int, int>> _sec_typeid_distrib;
        std::vector<std::string> _sec_idx2names;
    };
}