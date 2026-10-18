#pragma once
#include <array>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace neuron::morphology {

using Point3D = std::array<double, 3>;

std::pair<Point3D, std::vector<Point3D>> contourcenter(const std::vector<Point3D>& xyz);

std::pair<std::vector<Point3D>, std::vector<double>> contour2centroid(
    const Point3D& mean,
    const std::vector<Point3D>& points);

class MorphIOWrapper {
  public:
    explicit MorphIOWrapper(const std::string& filename);
    MorphIOWrapper(const MorphIOWrapper&) = delete;
    MorphIOWrapper& operator=(const MorphIOWrapper&) = delete;
    MorphIOWrapper(MorphIOWrapper&&) noexcept;
    MorphIOWrapper& operator=(MorphIOWrapper&&) noexcept;
    ~MorphIOWrapper();

    std::vector<std::string> morph_as_hoc() const;

    // Getter for _section_index_to_name
    inline const std::vector<std::string>& section_index_to_name() const {
        return _section_index_to_name;
    }

    // Getter for _section_type_distribution
    inline const std::vector<std::tuple<int, int, int>>& section_type_distribution() const {
        return _section_type_distribution;
    }

  private:
    struct MorphIOWrapperImpl;
    std::unique_ptr<MorphIOWrapperImpl> _pimpl;
    std::vector<std::string> _section_index_to_name;
    std::vector<std::tuple<int, int, int>> _section_type_distribution;
};
}  // namespace neuron::morphology
