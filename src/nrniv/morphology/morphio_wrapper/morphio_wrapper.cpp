#include "morphology/morphology.hpp"
#include "morphio_wrapper.hpp"
#include <morphio/morphology.h>
#include <morphio/section.h>
#include <morphio/soma.h>
#include <fmt/core.h>

namespace neuron::morphology {

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
                for (size_t i = soma_points.size(); i > 0; --i) {
                    const auto& p = soma_points[i - 1];
                    const auto& d = soma_diameters[i - 1];
                    cmds.emplace_back("soma { pt3dadd(" + fmt::format("{:.8g}", p[0]) + ", " + fmt::format("{:.8g}", p[1]) + ", " + fmt::format("{:.8g}", p[2]) + ", " + fmt::format("{:.8g}", d) + ") }");
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