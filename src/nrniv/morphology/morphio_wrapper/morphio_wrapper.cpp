#include "morphology/morphology.hpp"
#include "morphio_wrapper.hpp"
#include <morphio/morphology.h>
#include <morphio/section.h>
#include <morphio/soma.h>
#include <fmt/core.h>

namespace neuron::morphology {

    morphio::Point operator-(const morphio::Point& lhs, const morphio::Point& rhs) {
        return morphio::Point{lhs[0] - rhs[0], lhs[1] - rhs[1], lhs[2] - rhs[2]};
    }

    morphio::floatType norm(morphio::Point p) {
        return sqrt(p[0]*p[0] + p[1]*p[1]);
    }

    std::vector<morphio::floatType> norm(morphio::range<const morphio::Point> points) {
        std::vector<morphio::floatType> norms(points.size());
        std::transform(points.begin(), points.end(), norms.begin(), [](morphio::Point p) {
            return norm(p);
        });
        return norms;
    }

    std::pair<std::vector<morphio::Point>, std::vector<morphio::floatType>> single_point_sphere_to_circular_contour(morphio::Morphology& neuron) {
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

    std::pair<morphio::Point, std::vector<morphio::Point>> contourcenter(std::vector<morphio::Point> xyz) {
        const int POINTS = 101;

        std::vector<morphio::Point> points(xyz.size());
        std::adjacent_difference(xyz.begin(), xyz.end(), points.begin(), [](morphio::Point a, morphio::Point b) {
            return morphio::Point{b[0]-a[0], b[1]-a[1], 0.0};
        });
        points[0] = points.back();
        std::vector<morphio::floatType> perim = norm(points);
        std::partial_sum(perim.begin(), perim.end(), perim.begin());

        std::vector<morphio::floatType> d(POINTS);
        std::iota(d.begin(), d.end(), 0);
        std::transform(d.begin(), d.end(), d.begin(), [&](morphio::floatType x) {
            return x * perim.back() / (POINTS - 1);
        });

        std::vector<morphio::Point> new_xyz(POINTS, morphio::Point{0, 0, 0});
        std::transform(d.begin(), d.end(), new_xyz.begin(), [&](morphio::floatType x) {
            morphio::Point result{0, 0, 0};
            auto it = std::lower_bound(perim.begin(), perim.end(), x);
            int j = it - perim.begin();
            if (j == 0)
                result = xyz.front();
            else if (j == perim.size())
                result = xyz.back();
            else {
                morphio::floatType t = (x - perim[j-1]) / (perim[j] - perim[j-1]);
                for (int i = 0; i < 3; i++) {
                    result[i] = xyz[j-1][i] + t * (xyz[j][i] - xyz[j-1][i]);
                }
            }
            return result;
        });

        morphio::Point mean{0, 0, 0};
        for (int i = 0; i < 3; i++) {
            mean[i] = std::accumulate(new_xyz.begin(), new_xyz.end(), 0.0, [&](morphio::floatType a, morphio::Point p) {
                return a + p[i];
            }) / POINTS;
        }

        return {mean, new_xyz};
    }


    template <typename T>
    T interp1d(const std::vector<T>& x, const std::vector<T>& y, T xi) {
        auto it = std::lower_bound(x.begin(), x.end(), xi);
        if (it == x.begin()) {
            return y.front();
        } else if (it == x.end()) {
            return y.back();
        } else {
            int i = std::distance(x.begin(), it) - 1;
            T x0 = x[i];
            T x1 = x[i + 1];
            T y0 = y[i];
            T y1 = y[i + 1];
            return y0 + (y1 - y0) * (xi - x0) / (x1 - x0);
        }
    }

    std::pair<std::vector<morphio::Point>, std::vector<morphio::floatType>> contour2centroid(morphio::Point mean, std::vector<morphio::Point> points) {
        // find the major axis of the ellipsoid that best fits the shape
        // assuming (falsely in general) that the center is the mean

        std::vector<morphio::Point> new_points(points.size());
        std::transform(points.begin(), points.end(), new_points.begin(), [&](morphio::Point p) {
            return p - mean;
        });

        morphio::floatType eigen_values[3];
        morphio::floatType eigen_vectors[3][3];
        morphio::floatType covariance_matrix[3][3] = {0};

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                for (int k = 0; k < points.size(); k++) {
                    covariance_matrix[i][j] += new_points[k][i] * new_points[k][j];
                }
                covariance_matrix[i][j] /= points.size();
            }
        }

        morphio::floatType max_eigen_value = -1;
        int max_eigen_value_index = -1;
        for (int i = 0; i < 3; i++) {
            eigen_values[i] = covariance_matrix[i][i];
            if (eigen_values[i] > max_eigen_value) {
                max_eigen_value = eigen_values[i];
                max_eigen_value_index = i;
            }
        }

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                eigen_vectors[i][j] = (i == j) ? 1 : 0;
            }
        }

        for (int i = 0; i < 3; i++) {
            if (i == max_eigen_value_index) {
                continue;
            }

            morphio::floatType a = covariance_matrix[i][i];
            morphio::floatType b = covariance_matrix[i][max_eigen_value_index];
            morphio::floatType c = covariance_matrix[max_eigen_value_index][max_eigen_value_index];

            morphio::floatType lambda = (a + c - std::sqrt((a - c) * (a - c) + 4 * b * b)) / 2;

            eigen_values[i] = lambda;
            eigen_vectors[i][0] = -b / (a - lambda);
            eigen_vectors[i][1] = 1;
            eigen_vectors[i][2] = 0;
            morphio::floatType norm = std::sqrt(eigen_vectors[i][0] * eigen_vectors[i][0] + eigen_vectors[i][1] * eigen_vectors[i][1]);
            eigen_vectors[i][0] /= norm;
            eigen_vectors[i][1] /= norm;
        }

        morphio::Point major_axis{eigen_vectors[max_eigen_value_index][0], eigen_vectors[max_eigen_value_index][1], eigen_vectors[max_eigen_value_index][2]};
        morphio::Point minor_axis{eigen_vectors[0][0], eigen_vectors[0][1], eigen_vectors[0][2]};

        for (int i = 1; i < 3; i++) {
            if (eigen_values[i] > eigen_values[max_eigen_value_index]) {
                minor_axis = morphio::Point{eigen_vectors[i][0], eigen_vectors[i][1], eigen_vectors[i][2]};
            }
        }

        minor_axis[2] = 0;

        std::vector<morphio::floatType> major_coord(points.size());
        std::vector<morphio::floatType> minor_coord(points.size());
        std::transform(points.begin(), points.end(), major_coord.begin(), [&](morphio::Point p) {
            return p[0]*major_axis[0] + p[1]*major_axis[1] + p[2]*major_axis[2];;
        });
        std::transform(points.begin(), points.end(), minor_coord.begin(), [&](morphio::Point p) {
            return p[0]*minor_axis[0] + p[1]*minor_axis[1] + p[2]*minor_axis[2]; 
        });

        int imax = std::distance(major_coord.begin(), std::max_element(major_coord.begin(), major_coord.end()));
        std::rotate(major_coord.begin(), major_coord.begin() + imax, major_coord.end());
        std::rotate(minor_coord.begin(), minor_coord.begin() + imax, minor_coord.end());

        int imin = std::distance(major_coord.begin(), std::min_element(major_coord.begin(), major_coord.end()));

        std::vector<std::vector<morphio::floatType>> sides(2);
        std::vector<std::vector<morphio::floatType>> rads(2);

        sides[0].resize(imin);
        sides[1].resize(major_coord.size() - imin);
        rads[0].resize(imin);
        rads[1].resize(major_coord.size() - imin);

        std::copy(major_coord.begin(), major_coord.begin() + imin, sides[0].rbegin());
        std::copy(major_coord.begin() + imin, major_coord.end(), sides[1].begin());
        std::copy(minor_coord.begin(), minor_coord.begin() + imin, rads[0].rbegin());
        std::copy(minor_coord.begin() + imin, minor_coord.end(), rads[1].begin());

        for (int i_side = 0; i_side < 2; i_side++) {
            std::vector<morphio::floatType> m = sides[i_side];
            std::vector<morphio::floatType> idx(m.size(), true);

            morphio::floatType last_val = m.back();
            for (int i = m.size() - 2; i >= 0; i--) {
                if (m[i] < last_val) {
                    last_val = m[i];
                } else {
                    idx[i] = false;
                }
            }

            sides[i_side].erase(std::remove_if(sides[i_side].begin(), sides[i_side].end(), [&](morphio::floatType val) {
                if (&val < &sides[i_side][0] || &val >= &sides[i_side][sides[i_side].size()]) {
                    return false;
                }
                return !idx[&val - &sides[i_side][0]];
            }), sides[i_side].end());

            rads[i_side].erase(std::remove_if(rads[i_side].begin(), rads[i_side].end(), [&](morphio::floatType val) {
                if (&val < &rads[i_side][0] || &val >= &rads[i_side][rads[i_side].size()]) {
                    return false;
                }
                return !idx[&val - &rads[i_side][0]];
            }), rads[i_side].end());
        }

        std::vector<morphio::Point> new_return_points(21);
        std::vector<morphio::floatType> new_major_coord(21);
        std::vector<morphio::floatType> new_rads[2];

        std::iota(new_major_coord.begin(), new_major_coord.end(), 0);
        std::transform(new_major_coord.begin(), new_major_coord.end(), new_major_coord.begin(), [&](morphio::floatType val) {
            return major_coord.front() + val * (major_coord.back() - major_coord.front()) / (new_major_coord.size() - 1);
        });

        new_rads[0].resize(new_major_coord.size());
        new_rads[1].resize(new_major_coord.size());

        std::transform(new_major_coord.begin(), new_major_coord.end(), new_rads[0].begin(), [&](morphio::floatType val) {
        if (sides[0].empty() || rads[0].empty()) {
                return 0.0f;
            }
            return interp1d(sides[0], rads[0], val);
        });

        std::transform(new_major_coord.begin(), new_major_coord.end(), new_rads[1].begin(), [&](morphio::floatType val) {
            if (sides[1].empty() || rads[1].empty()) {
                return 0.0f;
            }
            return interp1d(sides[1], rads[1], val);
        });

        std::transform(new_major_coord.begin(), new_major_coord.end(), new_return_points.begin(), [&](morphio::floatType val) {
            return morphio::Point{major_axis[0]*val + mean[0], major_axis[1]*val + mean[1], major_axis[2]*val + mean[2]};
        });

        std::vector<morphio::floatType> diameters(new_rads[0].size());
        std::transform(new_rads[0].begin(), new_rads[0].end(), new_rads[1].begin(), diameters.begin(), [](morphio::floatType a, morphio::floatType b) {
            return std::abs(a - b);
        });

        diameters.front() = (diameters[0] + diameters[1]) / 2.0;
        diameters.back() = (diameters[diameters.size() - 2] + diameters.back()) / 2.0;

        return {new_return_points, diameters};
    }

    MorphIOWrapper::MorphIOWrapper(const std::string& filename) :
        _morph{std::make_unique<morphio::Morphology>(filename, morphio::NRN_ORDER)},
        _sections{ _morph->sections()} {
                int id = 1;
                int count = 0;
                int prev_type = _sections[0].type();
                _sec_idx2names.emplace_back("soma");
                _sec_typeid_distrib.push_back({1, -1, 1});
                int idx_adjust = 0;
                int current_type = -1;
                int i = 0;
                for (const auto& sec : _sections) {
                    if (sec.type() == prev_type) {
                        ++count;
                    } else {
                        _sec_typeid_distrib.push_back({prev_type, id, count});
                        ++id;
                        count = 1;
                        prev_type = sec.type();
                    }
                    if (sec.type() != current_type) {
                        current_type = sec.type();
                        idx_adjust = i;
                    }
                    _sec_idx2names.emplace_back(neuron::morphology::name(current_type, i - idx_adjust));
                    ++i;
                }
                _sec_typeid_distrib.push_back({prev_type, id, count});
                //print _sec_typeid_distrib
                // for (const auto& [type_id, start_id, count] : _sec_typeid_distrib) {
                //     std::cout << "type_id: " << type_id << " start_id: " << start_id << " count: " << count << std::endl;
                // }

                //print _sec_idx2names
                // for (const auto& name : _sec_idx2names) {
                //     std::cout << name << std::endl;
                // }


    }

    // future improvement: stor_pt3d_vec 
    std::vector<std::string> MorphIOWrapper::morph_as_hoc() const {
                std::vector<std::string> cmds;

                // generate create commands
                for (const auto& [type_id, start_id, count] : _sec_typeid_distrib) {
                    std::string tstr = type2name(type_id);
                    std::string tstr1 = "create " + tstr + "[" + std::to_string(count) + "]";
                    cmds.push_back(tstr1);
                    tstr1 = mksubset(type_id, count, tstr);
                    cmds.push_back(tstr1);
                }

                cmds.emplace_back("forall all.append");

                // generate 3D soma points commands. Order is reversed wrt NEURON's soma points.
                const auto& soma_points = _morph->soma().points();
                const auto& soma_diameters = _morph->soma().diameters();

                // for (size_t i = soma_points.size(); i > 0; --i) {
                //     const auto& p = soma_points[i - 1];
                //     const auto& d = soma_diameters[i - 1];
                //     cmds.emplace_back("soma { pt3dadd(" + fmt::format("{:.8g}", p[0]) + ", " + fmt::format("{:.8g}", p[1]) + ", " + fmt::format("{:.8g}", p[2]) + ", " + fmt::format("{:.8g}", d) + ") }");
                // }

                // convert soma_points to std::vector<std::array<double, 3>>
                std::vector<morphio::Point> soma_points_vec;
                for (const auto& point : soma_points) {
                    soma_points_vec.push_back({point[0], point[1], point[2]});
                }

                switch (_morph->soma().type())
                {
                case morphio::SomaType::SOMA_SIMPLE_CONTOUR:
                    {
                        auto [mean, contour_xyz] = contourcenter(soma_points_vec);
                        auto [new_xyz, new_diams] = contour2centroid(mean, contour_xyz);
                        for (size_t i = new_xyz.size(); i > 0; --i) {
                            const auto& p = new_xyz[i - 1];
                            const auto& d = new_diams[i - 1];
                            cmds.emplace_back("soma { pt3dadd(" + fmt::format("{:.8g}", p[0]) + ", " + fmt::format("{:.8g}", p[1]) + ", " + fmt::format("{:.8g}", p[2]) + ", " + fmt::format("{:.8g}", d) + ") }");
                        }
                    }
                    break;
                case morphio::SomaType::SOMA_SINGLE_POINT:
                    {
                        auto [new_xyz, new_diams] = single_point_sphere_to_circular_contour(*_morph);
                        for (size_t i = new_xyz.size(); i > 0; --i) {
                            const auto& p = new_xyz[i - 1];
                            const auto& d = new_diams[i - 1];
                            cmds.emplace_back("soma { pt3dadd(" + fmt::format("{:.8g}", p[0]) + ", " + fmt::format("{:.8g}", p[1]) + ", " + fmt::format("{:.8g}", p[2]) + ", " + fmt::format("{:.8g}", d) + ") }");
                        }
                    }
                    break;
                default:
                    throw std::runtime_error("Soma type not supported: " + std::to_string(static_cast<int>(_morph->soma().type())));
                    break;
                }

                

                // generate sections connect + their respective 3D points commands
                auto i = 0;
                for_each(_sections.begin(), _sections.end(), [&cmds, &i, this](const morphio::Section& sec) {
                    const size_t index = i + 1;
                    std::string tstr = _sec_idx2names[index];

                    if (!sec.isRoot()) {
                        const auto& parent = sec.parent();
                        const size_t parent_index = parent.id() + 1;
                        std::string tstr1 = _sec_idx2names[parent_index] + " connect " + tstr + "(0), 1";
                        cmds.emplace_back(tstr1);
                        
                    } else {
                        std::string tstr1 = "soma connect " + tstr + "(0), 0.5";
                        cmds.emplace_back(tstr1);
                    }

                    // 3D point info
                    const auto& points = sec.points();
                    const auto& diameters = sec.diameters();
                    for (size_t j = 0; j < points.size(); ++j) {
                        const auto& p = points[j];
                        const auto& d = diameters[j];
                        cmds.emplace_back(tstr + " { pt3dadd(" + fmt::format("{:.8g}", p[0]) + ", " + fmt::format("{:.8g}", p[1]) + ", " + fmt::format("{:.8g}", p[2]) + ", " + fmt::format("{:.8g}", d) + ") }");
                    }
                    ++i;
                });

                return cmds;
            }
}