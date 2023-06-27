#pragma once
#include <string>
#include <vector>
#include <memory>
#include <tuple>

namespace neuron::morphology {
class MorphIOWrapper {
    public:
        explicit MorphIOWrapper(const std::string& filename);
        MorphIOWrapper(const MorphIOWrapper&) = delete;
        MorphIOWrapper& operator=(const MorphIOWrapper&) = delete;
        MorphIOWrapper(MorphIOWrapper&&);
        MorphIOWrapper& operator=(MorphIOWrapper&&);
        ~MorphIOWrapper();

        std::vector<std::string> morph_as_hoc() const;

        // Getter for _sec_idx2names
        inline const std::vector<std::string>& sec_idx2names() const {
            return _sec_idx2names;
        }

        // Getter for _sec_typeid_distrib
        inline const std::vector<std::tuple<int, int, int>>& sec_typeid_distrib() const {
            return _sec_typeid_distrib;
        }

        private:
            struct MorphIOWrapperImpl;
            std::unique_ptr<MorphIOWrapperImpl> _pimpl;
            std::vector<std::string> _sec_idx2names;
            std::vector<std::tuple<int, int, int>> _sec_typeid_distrib;
        };
}  // namespace neuron::morphology
