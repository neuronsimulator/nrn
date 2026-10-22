#pragma once
#include <array>
#include <map>
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

    const std::string& filename() const {
        return _filename;
    }

    std::vector<std::string> to_hoc() const;

    // Getter for section names
    inline const std::vector<std::string>& section_names() const {
        return _section_names;
    }

    // Section count by type name: {"soma": 1, "axon": 3, ...}
    std::map<std::string, int> section_counts() const;

    // Getter for _section_type_distribution
    inline const std::vector<std::tuple<int, int, int>>& section_type_distribution() const {
        return _section_type_distribution;
    }

  private:
    struct MorphIOWrapperImpl;
    std::string _filename;
    std::unique_ptr<MorphIOWrapperImpl> _pimpl;
    std::vector<std::string> _section_names;
    std::vector<std::tuple<int, int, int>> _section_type_distribution;
};
}  // namespace neuron::morphology
