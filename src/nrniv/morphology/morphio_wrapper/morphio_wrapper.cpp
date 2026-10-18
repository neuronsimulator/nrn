#include "morphology/morphology.hpp"
#include "morphio_wrapper.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <unordered_map>
#include <Eigen/Dense>
#include <Eigen/Eigenvalues>
#include <morphio/morphology.h>
#include <morphio/section.h>
#include <morphio/soma.h>
#include <fmt/core.h>

namespace neuron::morphology {

class MorphologyError: public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

struct MorphIOWrapper::MorphIOWrapperImpl {
    explicit MorphIOWrapperImpl(morphio::Morphology&& morph)
        : _morph{std::move(morph)}
        , _sections{_morph.sections()} {}
    ~MorphIOWrapperImpl() = default;
    morphio::Morphology _morph;
    std::vector<morphio::Section> _sections;
};

MorphIOWrapper::MorphIOWrapper(MorphIOWrapper&&) noexcept = default;
MorphIOWrapper& MorphIOWrapper::operator=(MorphIOWrapper&&) noexcept = default;
MorphIOWrapper::~MorphIOWrapper() = default;

namespace {

// Matches HOC Vector.interpolate implementation from ivocvect
std::vector<double> hoc_interpolate(const std::vector<double>& xd,
                                    const std::vector<double>& xs,
                                    const std::vector<double>& ys) {
    const auto ns = static_cast<int>(xs.size());
    const auto nd = static_cast<int>(xd.size());
    std::vector<double> yd(nd, 0.0);
    if (ns == 0 || nd == 0) {
        return yd;
    }
    int id = 0;
    for (id = 0; id < nd && xd[id] <= xs[0]; ++id) {
        yd[id] = ys[0];
    }
    for (int is = 1; is < ns && id < nd; ++is) {
        if (xs[is] <= xs[is - 1]) {
            continue;
        }
        while (xd[id] <= xs[is]) {
            const double thet = (xd[id] - xs[is - 1]) / (xs[is] - xs[is - 1]);
            yd[id] = (1.0 - thet) * ys[is - 1] + thet * ys[is];
            ++id;
            if (id >= nd) {
                break;
            }
        }
    }
    for (; id < nd; ++id) {
        yd[id] = ys[ns - 1];
    }
    return yd;
}

}  // namespace

std::pair<std::vector<morphio::Point>, std::vector<morphio::floatType>>
single_point_sphere_to_circular_contour(morphio::Morphology& neuron) {
    /* Transform a single point soma that represents a sphere
     * into a circular contour that represents the same sphere */
    morphio::floatType radius = neuron.soma().diameters()[0] / 2.0;
    constexpr int N = 20;
    std::vector<morphio::Point> points(N);
    morphio::floatType phase;
    const auto& soma_points = neuron.soma().points();
    for (int i = 0; i < N; i++) {
        phase = 2.0 * M_PI / (N - 1) * i;
        points[i][0] = radius * cos(phase) + soma_points[0][0];
        points[i][1] = radius * sin(phase) + soma_points[0][1];
        points[i][2] = soma_points[0][2];
    }
    return std::make_pair(points, std::vector<morphio::floatType>(N, radius));
}

// C++ implementation of HOC Import3d_Section.contourcenter
// (share/lib/hoc/import3d/import3d_sec.hoc)
std::pair<Point3D, std::vector<Point3D>> contourcenter(const std::vector<Point3D>& xyz) {
    constexpr int POINTS = 101;
    const size_t n = xyz.size();
    if (n == 0) {
        return {Point3D{0.0, 0.0, 0.0}, {}};
    }

    std::vector<double> perim(n, 0.0);
    for (size_t i = 1; i < n; ++i) {
        const double dx = xyz[i][0] - xyz[i - 1][0];
        const double dy = xyz[i][1] - xyz[i - 1][1];
        perim[i] = perim[i - 1] + std::hypot(dx, dy);
    }

    std::vector<double> d(POINTS);
    const double step = perim.back() / (POINTS - 1);
    for (int i = 0; i < POINTS; ++i) {
        d[i] = i * step;
    }

    std::vector<Point3D> new_xyz(POINTS, Point3D{0.0, 0.0, 0.0});
    for (int c = 0; c < 3; ++c) {
        std::vector<double> coord(n);
        for (size_t i = 0; i < n; ++i) {
            coord[i] = xyz[i][c];
        }
        auto interp_c = hoc_interpolate(d, perim, coord);
        for (int i = 0; i < POINTS; ++i) {
            new_xyz[i][c] = interp_c[i];
        }
    }

    Point3D mean{0.0, 0.0, 0.0};
    for (int i = 0; i < POINTS; ++i) {
        for (int c = 0; c < 3; ++c) {
            mean[c] += new_xyz[i][c];
        }
    }
    for (int c = 0; c < 3; ++c) {
        mean[c] /= POINTS;
    }

    return {mean, new_xyz};
}

// C++ implementation of HOC Import3d_GUI.contour2centroid (share/lib/hoc/import3d/import3d_gui.hoc)
std::pair<std::vector<Point3D>, std::vector<double>> contour2centroid(
    const Point3D& mean,
    const std::vector<Point3D>& points) {
    const size_t n = points.size();
    if (n < 3) {
        throw MorphologyError("Failed to compute soma centroid from contour.");
    }

    // Center contour around mean
    Eigen::MatrixXd pts(3, n);
    for (size_t i = 0; i < n; ++i) {
        pts(0, i) = points[i][0] - mean[0];
        pts(1, i) = points[i][1] - mean[1];
        pts(2, i) = points[i][2] - mean[2];
    }

    // Find the major and minor axes of the 3x3 symmetric covariance matrix
    // SelfAdjointEigenSolver returns eigenvalues in ascending order (like numpy.linalg.eigh
    // in openbraininstitute/neurodamus): [0]=smallest, [1]=middle (minor), [2]=largest (major)
    const Eigen::Matrix3d m = pts * pts.transpose();
    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> es(m);
    const Eigen::Matrix3d eigvecs = es.eigenvectors();

    Eigen::Vector3d major = eigvecs.col(2);
    // Orientation heuristic from import3d_gui.hoc: major axis points in positive direction
    Eigen::Index abs_max_ind = 0;
    major.cwiseAbs().maxCoeff(&abs_max_ind);
    if (major(abs_max_ind) < 0) {
        major = -major;
    }

    // Minor axis is normal and in xy plane
    Eigen::Vector3d minor = eigvecs.col(1);
    minor(2) = 0.0;
    if (minor.norm() / (major.norm() + 1e-100) < 1e-6) {
        throw MorphologyError("Failed to compute soma centroid from contour.");
    }
    minor.normalize();

    std::vector<double> d(n);
    std::vector<double> rad(n);
    for (size_t i = 0; i < n; ++i) {
        const Eigen::Vector3d pt = pts.col(i);
        d[i] = pt.dot(major);
        rad[i] = pt.dot(minor);
    }

    const auto imax = std::distance(d.begin(), std::max_element(d.begin(), d.end()));
    std::rotate(d.begin(), d.begin() + imax, d.end());
    std::rotate(rad.begin(), rad.begin() + imax, rad.end());

    const auto imin = std::distance(d.begin(), std::min_element(d.begin(), d.end()));
    std::vector<double> side2(d.begin() + imin, d.end());
    std::vector<double> rad2(rad.begin() + imin, rad.end());
    d.resize(imin);
    rad.resize(imin);
    std::reverse(d.begin(), d.end());
    std::reverse(rad.begin(), rad.end());

    // Keep only points that make each side monotonically increasing (convex)
    auto make_convex = [](std::vector<double>& side, std::vector<double>& r) {
        for (int j = static_cast<int>(side.size()) - 1; j > 0; --j) {
            if (side[j] <= side[j - 1] + 1e-11) {
                side.erase(side.begin() + j);
                r.erase(r.begin() + j);
                if (j != static_cast<int>(side.size())) {
                    ++j;
                }
            }
        }
    };
    make_convex(d, rad);
    make_convex(side2, rad2);

    std::vector<double> tobj = d;
    tobj.insert(tobj.end(), side2.begin(), side2.end());
    std::sort(tobj.begin(), tobj.end());
    if (tobj.size() < 4) {
        throw MorphologyError("Failed to compute soma centroid from contour.");
    }

    const double x_start = tobj[1];
    const double x_end = tobj[tobj.size() - 2];
    constexpr int N_OUT = 21;
    std::vector<double> d_grid(N_OUT);
    for (int i = 0; i < N_OUT; ++i) {
        d_grid[i] = x_start + i * (x_end - x_start) / (N_OUT - 1);
    }

    const auto rad_interp = hoc_interpolate(d_grid, d, rad);
    const auto rad2_interp = hoc_interpolate(d_grid, side2, rad2);

    std::vector<Point3D> new_return_points(N_OUT);
    std::vector<double> diameters(N_OUT);
    const Eigen::Vector3d mean_vec(mean[0], mean[1], mean[2]);
    for (int i = 0; i < N_OUT; ++i) {
        const Eigen::Vector3d pt = major * d_grid[i] + mean_vec;
        new_return_points[i] = {pt(0), pt(1), pt(2)};
        diameters[i] = std::abs(rad_interp[i] - rad2_interp[i]);
    }

    // Avoid 0 diameter ends
    diameters.front() = (diameters[0] + diameters[1]) / 2.0;
    diameters.back() = (diameters[N_OUT - 2] + diameters[N_OUT - 1]) / 2.0;

    return {new_return_points, diameters};
}

MorphIOWrapper::MorphIOWrapper(const std::string& filename)
    : _pimpl{
          std::make_unique<MorphIOWrapperImpl>(morphio::Morphology(filename, morphio::NRN_ORDER))} {
    _section_index_to_name.emplace_back("soma");
    _section_type_distribution.push_back({1, -1, 1});
    if (!_pimpl->_sections.empty()) {
        std::vector<int> type_order;
        std::unordered_map<int, int> type_counts;
        for (const auto& sec: _pimpl->_sections) {
            const int t = sec.type();
            if (type_counts.find(t) == type_counts.end()) {
                type_order.push_back(t);
                type_counts[t] = 0;
            }
            ++type_counts[t];
        }
        int id = 1;
        for (const int t: type_order) {
            _section_type_distribution.push_back({t, id, type_counts[t]});
            ++id;
        }
        std::unordered_map<int, int> type_next_index;
        for (const auto& sec: _pimpl->_sections) {
            const int t = sec.type();
            const int idx = type_next_index[t]++;
            _section_index_to_name.emplace_back(neuron::morphology::name(t, idx));
        }
    }
}

// future improvement: stor_pt3d_vec
std::vector<std::string> MorphIOWrapper::morph_as_hoc() const {
    std::vector<std::string> cmds;

    // generate create commands
    for (const auto& [type_id, start_id, count]: _section_type_distribution) {
        const std::string tstr = type2name(type_id);
        cmds.push_back(fmt::format("create {}[{}]", tstr, count));
        cmds.push_back(mksubset(type_id, count, tstr));
    }

    cmds.emplace_back("forall all.append");

    // generate 3D soma points commands
    const auto& soma_points = _pimpl->_morph.soma().points();
    const auto& soma_diameters = _pimpl->_morph.soma().diameters();

    std::vector<Point3D> soma_points_vec;
    soma_points_vec.reserve(soma_points.size());
    for (const auto& point: soma_points) {
        soma_points_vec.push_back({point[0], point[1], point[2]});
    }

    switch (_pimpl->_morph.soma().type()) {
    case morphio::SomaType::SOMA_SIMPLE_CONTOUR: {
        auto [mean, contour_xyz] = contourcenter(soma_points_vec);
        auto [new_xyz, new_diams] = contour2centroid(mean, contour_xyz);
        for (size_t i = 0; i < new_xyz.size(); ++i) {
            const auto& p = new_xyz[i];
            const auto& d = new_diams[i];
            cmds.push_back(fmt::format(
                "soma {{ pt3dadd({:.8g}, {:.8g}, {:.8g}, {:.8g}) }}", p[0], p[1], p[2], d));
        }
    } break;
    case morphio::SomaType::SOMA_SINGLE_POINT: {
        auto [new_xyz, new_diams] = single_point_sphere_to_circular_contour(_pimpl->_morph);
        for (size_t i = new_xyz.size(); i > 0; --i) {
            const auto& p = new_xyz[i - 1];
            const auto& d = new_diams[i - 1];
            cmds.push_back(fmt::format(
                "soma {{ pt3dadd({:.8g}, {:.8g}, {:.8g}, {:.8g}) }}", p[0], p[1], p[2], d));
        }
    } break;
    case morphio::SomaType::SOMA_NEUROMORPHO_THREE_POINT_CYLINDERS: {
        const double x0 = soma_points[0][0];
        const double y0 = soma_points[0][1];
        const double z0 = soma_points[0][2];
        const double d0 = soma_diameters[0];
        const double r0 = d0 / 2.0;
        for (const double x: {x0 - r0, x0, x0 + r0}) {
            cmds.push_back(
                fmt::format("soma {{ pt3dadd({:.8g}, {:.8g}, {:.8g}, {:.8g}) }}", x, y0, z0, d0));
        }
    } break;
    case morphio::SomaType::SOMA_CYLINDERS: {
        for (size_t i = 0; i < soma_points.size(); ++i) {
            const auto& p = soma_points[i];
            const auto& d = soma_diameters[i];
            cmds.push_back(fmt::format(
                "soma {{ pt3dadd({:.8g}, {:.8g}, {:.8g}, {:.8g}) }}", p[0], p[1], p[2], d));
        }
    } break;
    default:
        throw MorphologyError(fmt::format("Soma type not supported: {}",
                                          static_cast<int>(_pimpl->_morph.soma().type())));
    }

    // generate sections connect + their respective 3D points commands
    for (size_t sec_idx = 0; sec_idx < _pimpl->_sections.size(); ++sec_idx) {
        const auto& sec = _pimpl->_sections[sec_idx];
        const size_t index = sec_idx + 1;
        const std::string& tstr = _section_index_to_name[index];

        if (!sec.isRoot()) {
            const auto& parent = sec.parent();
            const size_t parent_index = parent.id() + 1;
            cmds.push_back(
                fmt::format("{} connect {}(0), 1", _section_index_to_name[parent_index], tstr));
        } else {
            cmds.push_back(fmt::format("soma connect {}(0), 0.5", tstr));
        }

        // 3D point info
        const auto& points = sec.points();
        const auto& diameters = sec.diameters();
        for (size_t j = 0; j < points.size(); ++j) {
            const auto& p = points[j];
            const auto& d = diameters[j];
            cmds.push_back(fmt::format(
                "{} {{ pt3dadd({:.8g}, {:.8g}, {:.8g}, {:.8g}) }}", tstr, p[0], p[1], p[2], d));
        }
    }

    return cmds;
}
}  // namespace neuron::morphology