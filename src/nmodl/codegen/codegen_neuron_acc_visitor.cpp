#include "codegen/codegen_neuron_acc_visitor.hpp"

#include "ast/all.hpp"
#include "ast/block.hpp"
#include "ast/function_call.hpp"
#include "visitors/visitor_utils.hpp"

#include <algorithm>
#include <cstring>
#include <optional>
#include <unordered_set>
#include <vector>

namespace nmodl {
namespace codegen {

namespace {

bool name_is_table_statement_float(const CodegenInfo& info, const std::string& name) {
    for (const auto& v: info.table_statement_variables) {
        if (v && v->get_name() == name) {
            return true;
        }
    }
    return false;
}

}  // namespace

bool CodegenNeuronAccVisitor::is_table_statement_float(const std::string& name) const {
    return name_is_table_statement_float(info, name);
}

void CodegenNeuronAccVisitor::collect_ast_names(const ast::Ast& node,
                                                std::unordered_set<std::string>& names) const {
    for (const auto& n: collect_nodes(node, {ast::AstNodeType::NAME})) {
        auto nm = n->get_node_name();
        if (!nm.empty()) {
            names.insert(std::move(nm));
        }
    }
}

std::unordered_set<int> CodegenNeuronAccVisitor::live_float_indices_for_kernel(
    BlockType type) const {
    if (live_float_indices_override_) {
        return *live_float_indices_override_;
    }

    std::unordered_set<std::string> names;
    auto collect = [&](const ast::Ast* node) {
        if (node) {
            collect_ast_names(*node, names);
        }
    };

    if (type == BlockType::State) {
        // STATE calls rates/vtrap (TABLE often not inlined) — include procedure AST.
        collect(info.nrn_state_block);
        for (const auto& block: info.matexp_blocks) {
            collect(block);
        }
        for (const auto& procedure: info.procedures) {
            collect(procedure);
        }
        for (const auto& function: info.functions) {
            collect(function);
        }
    } else if (type == BlockType::Equation) {
        // CURRENT live set = breakpoint body only (not rates temps).
        if (info.breakpoint_node) {
            collect(info.breakpoint_node);
        }
        for (const auto& block: info.before_after_blocks) {
            collect(block);
        }
        // g_unused / conductance is written by cur kernel after the body.
        names.insert(info.vectorize ? naming::CONDUCTANCE_UNUSED_VARIABLE
                                    : naming::CONDUCTANCE_VARIABLE);
        names.insert(naming::VOLTAGE_UNUSED_VARIABLE);
    } else if (type == BlockType::NetReceive) {
        if (info.net_receive_node) {
            collect(info.net_receive_node);
        }
    } else if (type == BlockType::Initial) {
        if (info.initial_node) {
            collect(info.initial_node);
        }
        for (const auto& procedure: info.procedures) {
            collect(procedure);
        }
        for (const auto& function: info.functions) {
            collect(function);
        }
    }

    std::unordered_set<int> indices;
    for (std::string name: names) {
        if (!info.artificial_cell && name == "v") {
            name = naming::VOLTAGE_UNUSED_VARIABLE;
        }
        // H4c: TABLE rates temps are stack/ref locals on STATE — not SoA present.
        if (type == BlockType::State && is_table_statement_float(name)) {
            continue;
        }
        // STATE uses stack v; do not present v_unused.
        if (type == BlockType::State && name == naming::VOLTAGE_UNUSED_VARIABLE) {
            continue;
        }
        try {
            indices.insert(position_of_float_var(name));
        } catch (...) {
            // Not a float SoA column.
        }
    }

    // If analysis found nothing (empty MOD edge cases), present all as before.
    if (indices.empty() && !codegen_float_variables.empty() && type != BlockType::State) {
        for (int i = 0; i < static_cast<int>(codegen_float_variables.size()); ++i) {
            indices.insert(i);
        }
    }
    return indices;
}

std::unordered_set<int> CodegenNeuronAccVisitor::live_dptr_indices_for_kernel(BlockType type) {
    std::unordered_set<std::string> used_names;
    auto collect = [&](const ast::Ast* node) {
        if (node) {
            collect_ast_names(*node, used_names);
        }
    };
    if (type == BlockType::State) {
        collect(info.nrn_state_block);
        for (const auto& procedure: info.procedures) {
            collect(procedure);
        }
        for (const auto& function: info.functions) {
            collect(function);
        }
        // Concentration mechs write ions in STATE via shadow updates not always
        // visible as NAME uses. If STATE ion_write_statements is non-empty, keep
        // full ion dptrs. HH STATE has none (ion writes only on CURRENT).
        if (!ion_write_statements(BlockType::State).empty()) {
            for (size_t i = 0; i < codegen_int_variables.size(); ++i) {
                const auto& var = codegen_int_variables[i];
                auto const sem = info.semantics[i].name;
                if (!(var.is_index || var.is_integer || var.is_vdata ||
                      sem == naming::POINTER_SEMANTIC)) {
                    used_names.insert(var.symbol->get_name());
                }
            }
        }
    } else if (type == BlockType::Equation) {
        if (info.breakpoint_node) {
            collect(info.breakpoint_node);
        }
        // CURRENT always needs full ion dptr surface: USEION shadows plus
        // numerical di/dv slots (dinadv/dikdv) not named in the BREAKPOINT AST.
        if (!info.ions.empty()) {
            for (size_t i = 0; i < codegen_int_variables.size(); ++i) {
                const auto& var = codegen_int_variables[i];
                auto const sem = info.semantics[i].name;
                if (!(var.is_index || var.is_integer || var.is_vdata ||
                      sem == naming::POINTER_SEMANTIC)) {
                    used_names.insert(var.symbol->get_name());
                }
            }
        }
        // Point process nrn_cur scales by node area (ppvar dptr), even if "area"
        // never appears as a NAME in the BREAKPOINT AST.
        if (info.point_process) {
            used_names.insert(naming::NODE_AREA_VARIABLE);
        }
    } else if (type == BlockType::NetReceive) {
        if (info.net_receive_node) {
            collect(info.net_receive_node);
        }
    } else {
        // Conservative: all ion dptrs.
        for (size_t i = 0; i < codegen_int_variables.size(); ++i) {
            const auto& var = codegen_int_variables[i];
            auto const sem = info.semantics[i].name;
            if (!(var.is_index || var.is_integer || var.is_vdata ||
                  sem == naming::POINTER_SEMANTIC)) {
                // Will filter below by semantics name if needed; include all non-index.
                used_names.insert(var.symbol->get_name());
            }
        }
    }

    std::unordered_set<int> indices;
    for (size_t i = 0; i < codegen_int_variables.size(); ++i) {
        const auto& var = codegen_int_variables[i];
        auto const sem = info.semantics[i].name;
        if (var.is_index || var.is_integer || var.is_vdata || sem == naming::POINTER_SEMANTIC) {
            continue;
        }
        // Exact match only (ion_ena / ena). Do not substring-match — "n" would
        // hit every ion column.
        const auto name = var.symbol->get_name();
        std::string bare = name;
        if (bare.rfind(naming::ION_VARNAME_PREFIX, 0) == 0) {
            bare = bare.substr(std::strlen(naming::ION_VARNAME_PREFIX));
        }
        if (used_names.count(name) || used_names.count(bare)) {
            indices.insert(static_cast<int>(i));
        }
    }
    return indices;
}

std::string CodegenNeuronAccVisitor::backend_name() const {
    return "C++-OpenAcc-NEURON";
}

void CodegenNeuronAccVisitor::print_standard_includes() {
    CodegenNeuronCppVisitor::print_standard_includes();
    printer->add_line("#include <neuron/gpu/offload.hpp>");
    printer->add_line("#include <neuron/gpu/net_send_buffer.hpp>");
    printer->add_line("#include <neuron/gpu/net_receive_buffer.hpp>");
    printer->add_line("#include <neuron/gpu/mechanism_phases.hpp>");
    printer->add_line("#include <neuron/gpu/sync.hpp>");
    printer->add_line("#include <neuron/gpu/download.hpp>");
    printer->add_line("#include <neuron/event_order.hpp>");
    // net_buf_receive needs complete model_sorted_token + nrn_ensure_model_data_are_sorted.
    printer->add_line("#include \"nrn_ansi.h\"");
    printer->add_line("#include \"neuron/model_data.hpp\"");
}

bool CodegenNeuronAccVisitor::host_only_parallel_block(BlockType type) const {
    // INITIAL: host for ion wrote_conc, net_send/net_move (queue API), RANDOM
    // (nrnran123 not device-callable yet), or nrn_ghk (host codata/celsius).
    if (type != BlockType::Initial) {
        return false;
    }
    if (info.require_wrote_conc || info.net_send_used || info.net_event_used ||
        info.nrn_ghk_used) {
        return true;
    }
    for (const auto& sem: info.semantics) {
        if (sem.name == naming::RANDOM_SEMANTIC) {
            return true;
        }
    }
    return false;
}

void CodegenNeuronAccVisitor::print_global_var_struct_decl() {
    CodegenNeuronCppVisitor::print_global_var_struct_decl();
    if (!info.artificial_cell && !codegen_global_variables.empty()) {
        auto const& global = global_struct_instance();
        printer->fmt_line("static bool {}_gpu_resident = false;", global);
        // Host TABLE rebuild / GLOBAL mutation sets this; kernels H→D only when true.
        // Avoids per-step update device of full globals (HH: 6×201 table doubles).
        printer->fmt_line("static bool {}_device_stale = true;", global);
    }
}

void CodegenNeuronAccVisitor::print_global_variable_enter_data_once() const {
    if (info.artificial_cell || codegen_global_variables.empty()) {
        return;
    }
    auto const& global = global_struct_instance();
    printer->fmt_push_block("if (nt->compute_gpu && !{}_gpu_resident)", global);
    printer->fmt_line("(void) nrn_target_copyin(&{}, 1);", global);
    printer->add_line(global + "_gpu_resident = true;");
    // copyin snapshots host; further host edits require stale→update.
    printer->add_line(global + "_device_stale = false;");
    printer->pop_block();
}

void CodegenNeuronAccVisitor::print_global_variable_device_update_annotation() const {
    if (info.artificial_cell || codegen_global_variables.empty()) {
        return;
    }
    // H4a: only H→D when host marked globals dirty (TABLE rebuild, etc.).
    // Prior codegen updated the full struct every CURRENT/STATE/JACOB launch.
    auto const& global = global_struct_instance();
    printer->fmt_push_block("if (nt->compute_gpu && {}_gpu_resident && {}_device_stale)",
                            global,
                            global);
    printer->fmt_line("nrn_pragma_acc(update device ({}))", global);
    printer->fmt_line("nrn_pragma_omp(target update to({}))", global);
    printer->add_line(global + "_device_stale = false;");
    printer->pop_block();
}

void CodegenNeuronAccVisitor::print_mark_global_device_stale() const {
    if (info.artificial_cell || codegen_global_variables.empty()) {
        return;
    }
    printer->fmt_line("{}_device_stale = true;", global_struct_instance());
}

void CodegenNeuronAccVisitor::print_after_host_table_rebuild() {
    print_mark_global_device_stale();
}

void CodegenNeuronAccVisitor::print_kernel_global_device_setup() {
    print_global_variable_enter_data_once();
}

void CodegenNeuronAccVisitor::print_kernel_instance_data_copyin() {
    // Mechanism SOA is uploaded once; kernels use present(_ml_arg, ...) only.
}

std::string CodegenNeuronAccVisitor::global_variable_name(const SymbolType& symbol,
                                                          bool use_instance) const {
    // Force-inlined STATE body: use present(hh_global) names (hand-edit shape).
    // inst.global is a host pointer and causes residual copyin under device calls.
    if (inlining_state_specialized_body_) {
        return CodegenNeuronCppVisitor::global_variable_name(symbol, false);
    }
    if (use_present_fp_indexing_ || use_instance) {
        return fmt::format("inst.{}->{}", naming::INST_GLOBAL_MEMBER, symbol->get_name());
    }
    return CodegenNeuronCppVisitor::global_variable_name(symbol, false);
}

void CodegenNeuronAccVisitor::print_present_fp_pointer_declarations() const {
    const auto codegen_float_variables_size = codegen_float_variables.size();
    for (int i = 0; i < codegen_float_variables_size; ++i) {
        printer->fmt_line("double* _present_fp_{0} = _lmc.template fpfield_ptr<{0}>();", i);
    }
}

void CodegenNeuronAccVisitor::print_present_fp_pointer_declarations_for(
    const std::unordered_set<int>& indices) const {
    std::vector<int> ordered(indices.begin(), indices.end());
    std::sort(ordered.begin(), ordered.end());
    for (int i: ordered) {
        printer->fmt_line("double* _present_fp_{0} = _lmc.template fpfield_ptr<{0}>();", i);
    }
}

void CodegenNeuronAccVisitor::print_present_dptr_pointer_declarations() const {
    std::unordered_set<int> all;
    for (size_t i = 0; i < codegen_int_variables.size(); ++i) {
        const auto& var = codegen_int_variables[i];
        auto const sem = info.semantics[i].name;
        if (var.is_index || var.is_integer || var.is_vdata || sem == naming::POINTER_SEMANTIC) {
            continue;
        }
        all.insert(static_cast<int>(i));
    }
    print_present_dptr_pointer_declarations_for(all);
}

void CodegenNeuronAccVisitor::print_present_dptr_pointer_declarations_for(
    const std::unordered_set<int>& indices) const {
    std::vector<int> ordered(indices.begin(), indices.end());
    std::sort(ordered.begin(), ordered.end());
    for (int i: ordered) {
        const auto& var = codegen_int_variables[static_cast<size_t>(i)];
        auto const sem = info.semantics[static_cast<size_t>(i)].name;
        if (var.is_index || var.is_integer || var.is_vdata || sem == naming::POINTER_SEMANTIC) {
            continue;
        }
        // GPU cache is the full-instance pdata table; host dptr_field_ptr is already
        // advanced by ml storage offset. When using the GPU table, also record
        // storage_offset so (*_present_dptr_i[id + base]) hits the right instance
        // (multi-thread: offset>0). Host path uses base=0.
        printer->fmt_line("double* const* _present_dptr_{0} = nullptr;", i);
        printer->fmt_line("int _present_dptr_base_{0} = 0;", i);
        printer->push_block(
            "if (nt->compute_gpu && "
            "neuron::mechanism::_get::gpu_pdata_ptr_cache(_sorted_token, _ml_arg->type()))");
        printer->fmt_line(
            "_present_dptr_{0} = "
            "neuron::mechanism::_get::gpu_pdata_ptr_cache(_sorted_token, _ml_arg->type())[{0}];",
            i);
        printer->fmt_line(
            "_present_dptr_base_{0} = static_cast<int>(_ml_arg->get_storage_offset());", i);
        printer->pop_block();
        printer->push_block("else");
        printer->fmt_line("_present_dptr_{0} = _lmc.template dptr_field_ptr<{0}>();", i);
        printer->pop_block();
    }
}

std::string CodegenNeuronAccVisitor::present_dptr_deviceptr_clause() const {
    std::unordered_set<int> all;
    for (size_t i = 0; i < codegen_int_variables.size(); ++i) {
        const auto& var = codegen_int_variables[i];
        auto const sem = info.semantics[i].name;
        if (var.is_index || var.is_integer || var.is_vdata || sem == naming::POINTER_SEMANTIC) {
            continue;
        }
        all.insert(static_cast<int>(i));
    }
    return present_dptr_deviceptr_clause_for(all);
}

std::string CodegenNeuronAccVisitor::present_dptr_deviceptr_clause_for(
    const std::unordered_set<int>& indices) const {
    std::vector<int> ordered(indices.begin(), indices.end());
    std::sort(ordered.begin(), ordered.end());
    std::vector<std::string> dptr_names;
    for (int i: ordered) {
        dptr_names.push_back(fmt::format("_present_dptr_{}", i));
    }
    if (dptr_names.empty()) {
        return {};
    }
    return fmt::format(" deviceptr({})", fmt::join(dptr_names, ", "));
}

CodegenNeuronAccVisitor::ParamVector CodegenNeuronAccVisitor::functor_params() {
    // Eigen Newton functors capture RANGE via present_fp members. Their bodies
    // come from DERIVATIVE, not PROCEDURE, so procedure_live_* is incomplete.
    // Keep full double* present_fp set (table temps stay SoA here).
    if (info.mod_suffix == "nothing") {
        return {};
    }
    ParamVector params;
    params.emplace_back("", fmt::format("{}&", instance_struct()), "", "inst");
    if (!info.artificial_cell) {
        params.emplace_back("", fmt::format("{}&", node_data_struct()), "", "node_data");
    }
    params.emplace_back("", "size_t", "", "id");
    params.emplace_back("", "Datum*", "", "_ppvar");
    params.emplace_back("", "Datum*", "", "_thread");
    if (!codegen_thread_variables.empty()) {
        params.emplace_back("", fmt::format("{}&", thread_variables_struct()), "", "_thread_vars");
    }
    params.emplace_back("", "NrnThread*", "", "nt");
    for (int i = 0; i < static_cast<int>(codegen_float_variables.size()); ++i) {
        params.emplace_back("", "double*", "", fmt::format("_present_fp_{}", i));
    }
    return params;
}

bool CodegenNeuronAccVisitor::procedure_safe_for_state_specialization(
    const ast::Block& node) const {
    // VERBATIM can index arbitrary present_fp / host symbols — keep general ABI.
    if (!collect_nodes(node, {ast::AstNodeType::VERBATIM}).empty()) {
        return false;
    }
    // Nested MOD procedure/function calls need their own thin ABI; Session A
    // only specializes leaf procedures (InlineVisitor already folds pure FUNCTIONs).
    for (const auto& call: collect_nodes(node, {ast::AstNodeType::FUNCTION_CALL})) {
        const auto cname = call->get_node_name();
        if (is_net_send(cname) || is_net_move(cname) || is_net_event(cname)) {
            return false;
        }
        if (defined_method(cname) && cname != node.get_node_name()) {
            return false;
        }
    }

    std::unordered_set<std::string> names;
    collect_ast_names(node, names);
    // Ion / POINTER / RANDOM reads need fat dptr/id surface → general only.
    for (const auto& nm: names) {
        try {
            auto const pos = position_of_int_var(nm);
            auto const sem = info.semantics[static_cast<size_t>(pos)].name;
            if (sem == naming::POINTER_SEMANTIC || sem == naming::RANDOM_SEMANTIC ||
                sem == naming::FOR_NETCON_SEMANTIC) {
                return false;
            }
            // Ion dptrs (ena, ik, …) — specialized STATE path must not need them.
            const auto& var = codegen_int_variables[static_cast<size_t>(pos)];
            if (!(var.is_index || var.is_integer || var.is_vdata)) {
                return false;
            }
        } catch (...) {
            // Not an int SoA column.
        }
    }
    return true;
}

std::unordered_set<int> CodegenNeuronAccVisitor::procedure_live_present_fp_indices(
    const ast::Block& node) const {
    std::unordered_set<std::string> names;
    collect_ast_names(node, names);
    // Procedure formal parameters are not SoA columns.
    for (const auto& p: node.get_parameters()) {
        if (p) {
            names.erase(p->get_node_name());
        }
    }
    std::unordered_set<int> indices;
    for (const auto& nm: names) {
        if (is_table_statement_float(nm)) {
            continue;  // double& stack / _kl_* refs, not present_fp
        }
        try {
            indices.insert(position_of_float_var(nm));
        } catch (...) {
            // Not a float SoA column (global, local, celsius, …).
        }
    }
    return indices;
}

std::vector<std::string> CodegenNeuronAccVisitor::procedure_table_temp_names(
    const ast::Block& node) const {
    std::unordered_set<std::string> names;
    collect_ast_names(node, names);
    std::vector<std::string> temps;
    for (const auto& v: info.table_statement_variables) {
        if (v && !v->is_array() && names.count(v->get_name()) != 0) {
            temps.push_back(v->get_name());
        }
    }
    return temps;
}

CodegenNeuronAccVisitor::ParamVector CodegenNeuronAccVisitor::state_specialized_method_parameters(
    const ast::Block& node) const {
    ParamVector params;
    params.emplace_back("", fmt::format("{}&", instance_struct()), "", "inst");
    for (const auto& tname: procedure_table_temp_names(node)) {
        params.emplace_back("", "double&", "", fmt::format("_kl_{}", tname));
    }
    // Only non-table RANGE columns the procedure body actually touches.
    auto live = procedure_live_present_fp_indices(node);
    std::vector<int> ordered(live.begin(), live.end());
    std::sort(ordered.begin(), ordered.end());
    for (int i: ordered) {
        params.emplace_back("", "double*", "", fmt::format("_present_fp_{}", i));
    }
    return params;
}

std::string CodegenNeuronAccVisitor::state_specialized_method_arguments(
    const std::string& proc_name) const {
    // Find procedure AST for live-set (same as emission).
    const ast::Block* proc = nullptr;
    for (const auto* p: info.procedures) {
        if (p && p->get_node_name() == proc_name) {
            proc = p;
            break;
        }
    }
    for (const auto* f: info.functions) {
        if (f && f->get_node_name() == proc_name) {
            proc = f;
            break;
        }
    }
    std::vector<std::string> args;
    args.emplace_back("inst");
    if (proc) {
        for (const auto& tname: procedure_table_temp_names(*proc)) {
            // STATE stack locals use bare names; nested specialized body uses _kl_*.
            if (state_kernel_locals_active_) {
                args.push_back(tname);
            } else {
                args.push_back(fmt::format("_kl_{}", tname));
            }
        }
        auto live = procedure_live_present_fp_indices(*proc);
        std::vector<int> ordered(live.begin(), live.end());
        std::sort(ordered.begin(), ordered.end());
        for (int i: ordered) {
            args.push_back(fmt::format("_present_fp_{}", i));
        }
    }
    return fmt::format("{}", fmt::join(args, ", "));
}

bool CodegenNeuronAccVisitor::state_kernel_uses_only_specialized_procedures() const {
    // Only slim present_fp decls when we actually emitted specialized
    // procedures. Vacuous true (no calls) would drop columns needed by
    // non-procedure STATE bodies (arrays, functors, etc.).
    if (state_specialized_procedures_.empty()) {
        return false;
    }
    std::unordered_set<std::string> called;
    auto collect_calls = [&](const ast::Ast* node) {
        if (!node) {
            return;
        }
        for (const auto& call: collect_nodes(*node, {ast::AstNodeType::FUNCTION_CALL})) {
            const auto cname = call->get_node_name();
            if (defined_method(cname)) {
                called.insert(cname);
            }
        }
    };
    collect_calls(info.nrn_state_block);
    for (const auto& block: info.matexp_blocks) {
        collect_calls(block);
    }
    // Must have at least one MOD call covered by specialization.
    if (called.empty()) {
        return false;
    }
    for (const auto& cname: called) {
        if (state_specialized_procedures_.count(cname) == 0) {
            return false;
        }
    }
    return true;
}

CodegenNeuronAccVisitor::ParamVector CodegenNeuronAccVisitor::internal_method_parameters() {
    if (info.mod_suffix == "nothing") {
        return {};
    }

    // Session A: thin ABI for rates_*_state / f_rates_*_state.
    if (emitting_state_specialized_procedure_) {
        // inst + TABLE double& + live present_fp only; node_data/id/_ppvar dropped.
        ParamVector params;
        params.emplace_back("", fmt::format("{}&", instance_struct()), "", "inst");
        for (const auto& tname: state_specialized_table_temps_) {
            params.emplace_back("", "double&", "", fmt::format("_kl_{}", tname));
        }
        if (state_specialized_live_fp_) {
            std::vector<int> ordered(state_specialized_live_fp_->begin(),
                                     state_specialized_live_fp_->end());
            std::sort(ordered.begin(), ordered.end());
            for (int i: ordered) {
                params.emplace_back("", "double*", "", fmt::format("_present_fp_{}", i));
            }
        }
        return params;
    }

    ParamVector params;
    params.emplace_back("", fmt::format("{}&", instance_struct()), "", "inst");
    if (!info.artificial_cell) {
        params.emplace_back("", fmt::format("{}&", node_data_struct()), "", "node_data");
    }
    params.emplace_back("", "size_t", "", "id");
    params.emplace_back("", "Datum*", "", "_ppvar");
    params.emplace_back("", "Datum*", "", "_thread");
    if (!codegen_thread_variables.empty()) {
        params.emplace_back("", fmt::format("{}&", thread_variables_struct()), "", "_thread_vars");
    }
    params.emplace_back("", "NrnThread*", "", "nt");
    // H4c: TABLE statement vars (rates temps) are double& so STATE can pass stack
    // locals and HOC/table-update can bind _present_fp_i[id]. Other RANGE stay
    // double* columns. General ABI keeps full present_fp for VERBATIM safety.
    for (int i = 0; i < static_cast<int>(codegen_float_variables.size()); ++i) {
        const auto& name = codegen_float_variables[static_cast<size_t>(i)]->get_name();
        if (is_table_statement_float(name)) {
            params.emplace_back("", "double&", "", fmt::format("_kl_{}", name));
        } else {
            params.emplace_back("", "double*", "", fmt::format("_present_fp_{}", i));
        }
    }
    return params;
}

std::string CodegenNeuronAccVisitor::internal_method_arguments() {
    if (info.mod_suffix == "nothing") {
        return {};
    }

    if (emitting_state_specialized_procedure_) {
        std::vector<std::string> args;
        args.emplace_back("inst");
        for (const auto& tname: state_specialized_table_temps_) {
            args.push_back(fmt::format("_kl_{}", tname));
        }
        if (state_specialized_live_fp_) {
            std::vector<int> ordered(state_specialized_live_fp_->begin(),
                                     state_specialized_live_fp_->end());
            std::sort(ordered.begin(), ordered.end());
            for (int i: ordered) {
                args.push_back(fmt::format("_present_fp_{}", i));
            }
        }
        return fmt::format("{}", fmt::join(args, ", "));
    }

    std::vector<std::string> args;
    args.emplace_back("inst");
    if (!info.artificial_cell) {
        args.emplace_back("node_data");
    }
    args.emplace_back("id");
    args.emplace_back("_ppvar");
    args.emplace_back("_thread");
    if (!codegen_thread_variables.empty()) {
        args.emplace_back("_thread_vars");
    }
    args.emplace_back("nt");
    for (int i = 0; i < static_cast<int>(codegen_float_variables.size()); ++i) {
        const auto& name = codegen_float_variables[static_cast<size_t>(i)]->get_name();
        if (is_table_statement_float(name)) {
            if (state_kernel_locals_active_) {
                args.push_back(name);
            } else if (use_kl_ref_in_float_name_) {
                args.push_back(fmt::format("_kl_{}", name));
            } else {
                args.push_back(fmt::format("_present_fp_{}[id]", i));
            }
        } else {
            args.push_back(fmt::format("_present_fp_{}", i));
        }
    }
    return fmt::format("{}", fmt::join(args, ", "));
}

void CodegenNeuronAccVisitor::print_state_specialized_function_declaration(
    const ast::Block& node,
    const std::string& cpp_method_name,
    const std::unordered_set<CppObjectSpecifier>& specifiers) {
    // Like print_function_declaration, but uses a fully-mangled name
    // (method_name(base) + "_state") instead of method_name(base + "_state").
    enable_variable_name_lookup = false;
    auto type = default_float_data_type();
    auto internal_params = internal_method_parameters();
    const auto& params = node.get_parameters();
    for (const auto& param: params) {
        internal_params.emplace_back("", type, "", param.get()->get_node_name());
    }
    const char* return_type = node.is_function_block() ? default_float_data_type() : "int";
    printer->add_indent();
    printer->fmt_text("{} {} {}({})",
                      get_object_specifiers(specifiers),
                      return_type,
                      cpp_method_name,
                      get_parameter_str(internal_params));
    enable_variable_name_lookup = true;
}

void CodegenNeuronAccVisitor::print_state_specialized_function_or_procedure(
    const ast::Block& node,
    const std::string& cpp_method_name) {
    printer->add_newline(2);
    print_state_specialized_function_declaration(
        node, cpp_method_name, {CppObjectSpecifier::Static, CppObjectSpecifier::Inline});
    printer->add_text(" ");
    printer->push_block();
    // ret_* uses a stable token (not the mangled C++ name).
    if (node.is_function_block()) {
        printer->fmt_line("{} ret_state = 0.0;", default_float_data_type());
    } else {
        printer->add_line("int ret_state = 0;");
    }
    // No dead node_data V load — thin ABI has no node_data; v is a formal.
    print_statement_block(*node.get_statement_block(), false, false);
    printer->add_line("return ret_state;");
    printer->pop_block();
}

void CodegenNeuronAccVisitor::print_state_specialized_table_replacement(const ast::Block& node) {
    // Same TABLE interpolation as print_table_replacement_function, but thin ABI
    // (inst + _kl_* + live present_fp) and calls f_*_hh_state.
    auto name = node.get_node_name();
    auto statement = get_table_statement(node);
    auto table_variables = statement->get_table_vars();
    auto with = statement->get_with()->eval();
    auto use_table_var = get_variable_name(naming::USE_TABLE_VARIABLE);
    auto tmin_name = get_variable_name("tmin_" + name);
    auto mfac_name = get_variable_name("mfac_" + name);
    // rates_hh_state / f_rates_hh_state (not rates_state_hh).
    auto function_name = method_name("f_" + name) + "_state";
    auto specialized_name = method_name(name) + "_state";

    printer->add_newline(2);
    print_state_specialized_function_declaration(
        node, specialized_name, {CppObjectSpecifier::Static, CppObjectSpecifier::Inline});
    printer->push_block();
    {
        const auto& params = node.get_parameters();
        printer->fmt_push_block("if ({} == 0)", use_table_var);
        if (node.is_procedure_block()) {
            printer->fmt_line("{}({}, {});",
                              function_name,
                              internal_method_arguments(),
                              params[0].get()->get_node_name());
            printer->add_line("return 0;");
        } else {
            printer->fmt_line("return {}({}, {});",
                              function_name,
                              internal_method_arguments(),
                              params[0].get()->get_node_name());
        }
        printer->pop_block();

        printer->fmt_line("double xi = {} * ({} - {});",
                          mfac_name,
                          params[0].get()->get_node_name(),
                          tmin_name);
        printer->push_block("if (isnan(xi))");
        if (node.is_procedure_block()) {
            for (const auto& var: table_variables) {
                auto var_name = get_variable_name(var->get_node_name());
                auto [is_array, array_length] = check_if_var_is_array(var->get_node_name());
                if (is_array) {
                    for (int j = 0; j < array_length; j++) {
                        printer->fmt_line("{}[{}] = xi;", var_name, j);
                    }
                } else {
                    printer->fmt_line("{} = xi;", var_name);
                }
            }
            printer->add_line("return 0;");
        } else {
            printer->add_line("return xi;");
        }
        printer->pop_block();

        printer->fmt_push_block("if (xi <= 0. || xi >= {}.)", with);
        printer->fmt_line("int index = (xi <= 0.) ? 0 : {};", with);
        if (node.is_procedure_block()) {
            for (const auto& variable: table_variables) {
                auto var_name = variable->get_node_name();
                auto instance_name = get_variable_name(var_name);
                auto table_name = get_variable_name("t_" + var_name);
                auto [is_array, array_length] = check_if_var_is_array(var_name);
                if (is_array) {
                    for (int j = 0; j < array_length; j++) {
                        printer->fmt_line(
                            "{}[{}] = {}[{}][index];", instance_name, j, table_name, j);
                    }
                } else {
                    printer->fmt_line("{} = {}[index];", instance_name, table_name);
                }
            }
            printer->add_line("return 0;");
        } else {
            auto table_name = get_variable_name("t_" + name);
            printer->fmt_line("return {}[index];", table_name);
        }
        printer->pop_block();

        printer->add_line("int i = int(xi);");
        printer->add_line("double theta = xi - double(i);");
        if (node.is_procedure_block()) {
            for (const auto& var: table_variables) {
                auto var_name = var->get_node_name();
                auto instance_name = get_variable_name(var_name);
                auto table_name = get_variable_name("t_" + var_name);
                auto [is_array, array_length] = check_if_var_is_array(var->get_node_name());
                if (is_array) {
                    for (size_t j = 0; j < array_length; j++) {
                        printer->fmt_line(
                            "{0}[{1}] = {2}[{1}][i] + theta*({2}[{1}][i+1]-{2}[{1}][i]);",
                            instance_name,
                            j,
                            table_name);
                    }
                } else {
                    printer->fmt_line("{0} = {1}[i] + theta*({1}[i+1]-{1}[i]);",
                                      instance_name,
                                      table_name);
                }
            }
            printer->add_line("return 0;");
        } else {
            auto table_name = get_variable_name("t_" + name);
            printer->fmt_line("return {0}[i] + theta * ({0}[i+1] - {0}[i]);", table_name);
        }
    }
    printer->pop_block();
}

bool CodegenNeuronAccVisitor::procedure_called_from_state(const std::string& proc_name) const {
    auto has_call = [&](const ast::Ast* node) {
        if (!node) {
            return false;
        }
        for (const auto& call: collect_nodes(*node, {ast::AstNodeType::FUNCTION_CALL})) {
            if (call->get_node_name() == proc_name) {
                return true;
            }
        }
        return false;
    };
    if (has_call(info.nrn_state_block)) {
        return true;
    }
    for (const auto& block: info.matexp_blocks) {
        if (has_call(block)) {
            return true;
        }
    }
    return false;
}

void CodegenNeuronAccVisitor::print_state_specialized_procedure_versions(const ast::Block& node) {
    // Session A: only PROCEDURE blocks invoked from STATE (e.g. rates). Do not
    // specialize FUNCTIONs (vtrap): print_function mutates the AST return name
    // to ret_<fn>, and thin FUNCTIONs are not the H4c residual.
    if (!node.is_procedure_block()) {
        return;
    }
    auto name = node.get_node_name();
    if (!procedure_called_from_state(name)) {
        return;
    }
    if (!procedure_safe_for_state_specialization(node)) {
        return;
    }
    state_specialized_live_fp_ = procedure_live_present_fp_indices(node);
    state_specialized_table_temps_ = procedure_table_temp_names(node);
    emitting_state_specialized_procedure_ = true;
    use_kl_ref_in_float_name_ = true;
    use_present_fp_indexing_ = true;

    if (info.function_uses_table(name)) {
        // Analytic: f_rates_hh_state(inst, double& temps..., v)
        print_state_specialized_function_or_procedure(node, method_name("f_" + name) + "_state");
        // TABLE path: rates_hh_state(...)
        print_state_specialized_table_replacement(node);
    } else {
        print_state_specialized_function_or_procedure(node, method_name(name) + "_state");
    }

    use_kl_ref_in_float_name_ = false;
    emitting_state_specialized_procedure_ = false;
    state_specialized_live_fp_.reset();
    state_specialized_table_temps_.clear();
    state_specialized_procedures_.insert(name);
}

void CodegenNeuronAccVisitor::print_function_or_procedure(
    const ast::Block& node,
    const std::string& name,
    const std::unordered_set<CppObjectSpecifier>& specifiers) {
    printer->add_newline(2);
    print_function_declaration(node, name, specifiers);
    printer->add_text(" ");
    printer->push_block();

    if (node.is_function_block()) {
        auto type = default_float_data_type();
        printer->fmt_line("{} ret_{} = 0.0;", type, name);
    } else {
        printer->fmt_line("int ret_{} = 0;", name);
    }

    // General path: dead host V load for VERBATIM/legacy.
    if (info.mod_suffix != "nothing" && !info.artificial_cell) {
        printer->add_line(
            "double v = node_data.node_voltages ? "
            "node_data.node_voltages[node_data.nodeindices[id]] : 0.0;");
    }

    print_statement_block(*node.get_statement_block(), false, false);
    printer->fmt_line("return ret_{};", name);
    printer->pop_block();
}

void CodegenNeuronAccVisitor::print_inlined_table_procedure_body(
    const ast::Block& node,
    const std::string& formal_v_name) {
    // Same TABLE path as print_state_specialized_table_replacement, but:
    // - no function wrapper / returns (embedded in STATE loop)
    // - writes stack TABLE temps via state_kernel_locals_active_
    // - globals via present(hh_global) when inlining_state_specialized_body_
    auto name = node.get_node_name();
    auto statement = get_table_statement(node);
    auto table_variables = statement->get_table_vars();
    auto with = statement->get_with()->eval();
    auto use_table_var = get_variable_name(naming::USE_TABLE_VARIABLE);
    auto tmin_name = get_variable_name("tmin_" + name);
    auto mfac_name = get_variable_name("mfac_" + name);

    printer->fmt_push_block("if ({} == 0)", use_table_var);
    print_statement_block(*node.get_statement_block(), false, false);
    printer->pop_block();

    printer->push_block("else");
    printer->fmt_line("double xi = {} * ({} - {});", mfac_name, formal_v_name, tmin_name);
    printer->push_block("if (isnan(xi))");
    for (const auto& var: table_variables) {
        auto var_name = get_variable_name(var->get_node_name());
        auto [is_array, array_length] = check_if_var_is_array(var->get_node_name());
        if (is_array) {
            for (int j = 0; j < array_length; j++) {
                printer->fmt_line("{}[{}] = xi;", var_name, j);
            }
        } else {
            printer->fmt_line("{} = xi;", var_name);
        }
    }
    printer->pop_block();

    printer->fmt_push_block("else if (xi <= 0. || xi >= {}.)", with);
    printer->fmt_line("int index = (xi <= 0.) ? 0 : {};", with);
    for (const auto& variable: table_variables) {
        auto var_name = variable->get_node_name();
        auto instance_name = get_variable_name(var_name);
        auto table_name = get_variable_name("t_" + var_name);
        auto [is_array, array_length] = check_if_var_is_array(var_name);
        if (is_array) {
            for (int j = 0; j < array_length; j++) {
                printer->fmt_line("{}[{}] = {}[{}][index];", instance_name, j, table_name, j);
            }
        } else {
            printer->fmt_line("{} = {}[index];", instance_name, table_name);
        }
    }
    printer->pop_block();

    printer->push_block("else");
    printer->add_line("int i = int(xi);");
    printer->add_line("double theta = xi - double(i);");
    for (const auto& var: table_variables) {
        auto var_name = var->get_node_name();
        auto instance_name = get_variable_name(var_name);
        auto table_name = get_variable_name("t_" + var_name);
        auto [is_array, array_length] = check_if_var_is_array(var->get_node_name());
        if (is_array) {
            for (size_t j = 0; j < array_length; j++) {
                printer->fmt_line(
                    "{0}[{1}] = {2}[{1}][i] + theta*({2}[{1}][i+1]-{2}[{1}][i]);",
                    instance_name,
                    j,
                    table_name);
            }
        } else {
            printer->fmt_line("{0} = {1}[i] + theta*({1}[i+1]-{1}[i]);",
                              instance_name,
                              table_name);
        }
    }
    printer->pop_block();  // else interpolate
    printer->pop_block();  // else usetable
}

void CodegenNeuronAccVisitor::print_state_specialized_call_force_inlined(
    const ast::FunctionCall& node) {
    // Session A residual: force-inline unique/safe STATE rates body (hand-edit).
    // Eliminates the device call + residual copyin/out under state_hh.
    const auto& name = node.get_node_name();
    const ast::Block* proc = nullptr;
    for (const auto* p: info.procedures) {
        if (p && p->get_node_name() == name) {
            proc = p;
            break;
        }
    }
    if (!proc) {
        // Should not happen for specialized names; fall back to thin call.
        printer->add_text(method_name(name) + "_state", '(');
        auto internal_args = state_specialized_method_arguments(name);
        printer->add_text(internal_args);
        const auto& arguments = node.get_arguments();
        if (!arguments.empty() && !internal_args.empty()) {
            printer->add_text(", ");
        }
        print_vector_elements(arguments, ", ");
        printer->add_text(')');
        return;
    }

    const auto& arguments = node.get_arguments();
    const auto& params = proc->get_parameters();

    // Statement context: caller wraps with `;` after expression statements.
    // Emit a compound statement; the trailing `;` after `}` is harmless C++.
    printer->push_block();

    // Bind MOD formals (e.g. rates(v) → _lv) to call-site args (stack `v`).
    std::string formal_v_name = "v";
    for (size_t i = 0; i < params.size(); ++i) {
        auto pname = params[i].get()->get_node_name();
        if (i == 0) {
            formal_v_name = pname;
        }
        printer->add_indent();
        printer->fmt_text("const double {} = ", pname);
        if (i < arguments.size()) {
            arguments[i]->accept(*this);
        } else {
            printer->add_text("0.0");
        }
        printer->add_text(';');
        printer->add_newline();
    }

    inlining_state_specialized_body_ = true;
    // TABLE temps already stack locals (state_kernel_locals_active_).
    // present_fp indexing still on for any non-table RANGE the body might touch.
    if (info.function_uses_table(name)) {
        print_inlined_table_procedure_body(*proc, formal_v_name);
    } else {
        print_statement_block(*proc->get_statement_block(), false, false);
    }
    inlining_state_specialized_body_ = false;

    printer->pop_block();
}

void CodegenNeuronAccVisitor::print_function_call(const ast::FunctionCall& node) {
    const auto& name = node.get_node_name();
    if (state_kernel_locals_active_ && defined_method(name) &&
        state_specialized_procedures_.count(name) != 0) {
        // Force-inline unique/safe STATE rates body (TABLE path preferred).
        // Thin rates_*_state remains available for analytic/debug; STATE does not call it.
        print_state_specialized_call_force_inlined(node);
        return;
    }
    CodegenCppVisitor::print_function_call(node);
}

void CodegenNeuronAccVisitor::print_function_procedure_helper(const ast::Block& node) {
    auto name = node.get_node_name();
    if (info.function_uses_table(name)) {
        auto new_name = "f_" + name;
        // f_rates / rates / table-update bodies: TABLE temps are _kl_* ref params.
        use_kl_ref_in_float_name_ = true;
        print_function_or_procedure(node,
                                    new_name,
                                    {CppObjectSpecifier::Static, CppObjectSpecifier::Inline});
        print_table_check_function(node);
        print_table_replacement_function(node);
        use_kl_ref_in_float_name_ = false;
    } else {
        use_kl_ref_in_float_name_ = true;
        print_function_or_procedure(node, name);
        use_kl_ref_in_float_name_ = false;
    }
    // Session A: also emit thin *_state version when safe (STATE hot path).
    print_state_specialized_procedure_versions(node);
}

void CodegenNeuronAccVisitor::print_function_definitions() {
    print_hoc_py_wrapper_function_definitions();
    // HOC wrappers done — procedure bodies pass _kl_* not SoA elements.
    hoc_wrapper_table_temp_as_soa_ = false;
    use_present_fp_indexing_ = true;
    for (const auto& procedure: info.procedures) {
        print_procedure(*procedure);
    }
    for (const auto& function: info.functions) {
        print_function(*function);
    }
    for (const auto& function_table: info.function_tables) {
        print_function_tables(*function_table);
    }
    use_present_fp_indexing_ = false;
}

void CodegenNeuronAccVisitor::print_functors_definitions() {
    // Functor params include _present_fp_* (internal_method_parameters) and the
    // state loop constructs them with those pointers. initialize()/operator()
    // must index via present_fp, not _lmc.template fpfield (no _lmc member).
    use_present_fp_indexing_ = true;
    CodegenCppVisitor::print_functors_definitions();
    use_present_fp_indexing_ = false;
}

void CodegenNeuronAccVisitor::print_hoc_py_wrapper_before_table_update() {
    // Include artificial cells (VecStim): ACC MOD FUNCTIONs take _present_fp_*.
    if (info.mod_suffix != "nothing" && !codegen_float_variables.empty()) {
        print_present_fp_pointer_declarations();
    }
    // H4c: HOC/Python bind TABLE temps as SoA elements into double& params.
    hoc_wrapper_table_temp_as_soa_ = true;
}



void CodegenNeuronAccVisitor::print_parallel_iteration_hint(BlockType type,
                                                            const ast::Block* block) {
    if (info.artificial_cell) {
        return;
    }

    if (host_only_parallel_block(type)) {
        CodegenNeuronCppVisitor::print_parallel_iteration_hint(type, block);
        return;
    }

    // Reuse CPU ivdep path when block uses mutex/protect (atomics + SIMD conflict).
    std::vector<std::shared_ptr<const ast::Ast>> nodes;
    if (block) {
        nodes = collect_nodes(*block,
                              {ast::AstNodeType::PROTECT_STATEMENT,
                               ast::AstNodeType::MUTEX_LOCK,
                               ast::AstNodeType::MUTEX_UNLOCK});
    }
    if (!nodes.empty()) {
        CodegenNeuronCppVisitor::print_parallel_iteration_hint(type, block);
        return;
    }

    print_global_variable_device_update_annotation();

    // H4c: per-kernel live SoA columns (named bases). OpenACC cannot track
    // array-of-pointers; only present columns the kernel actually touches.
    const auto live_fp = live_float_indices_for_kernel(type);
    // Jacob override is float-only (g_unused); no ion dptrs.
    const auto live_dptr = (type == BlockType::NetReceive || live_float_indices_override_)
                               ? std::unordered_set<int>{}
                               : live_dptr_indices_for_kernel(type);

    if (type == BlockType::NetReceive) {
        // Present pointers declared by print_net_receive_buffering before the loop.
    } else if (!info.artificial_cell) {
        printer->add_line("auto const* nodeindices = node_data.nodeindices;");
        // Device-resident voltages: use deviceptr, never present(host V).
        // present(node_voltages) re-uploads stale host V on nvc++ when host is
        // dirty/out of date — host must not participate in psolve V traffic.
        printer->add_line(
            "double* _d_voltages = nt->compute_gpu "
            "? static_cast<double*>(acc_deviceptr(const_cast<double*>(node_data.node_voltages))) "
            ": const_cast<double*>(node_data.node_voltages);");
        if (type == BlockType::Equation) {
            printer->add_line("double* vec_rhs = node_data.node_rhs;");
            printer->add_line("double* vec_d = node_data.node_diagonal;");
        }
        // Session A: when STATE only calls thin specialized procedures (e.g.
        // rates_*_state), declare live SoA columns only — no fat present_fp
        // args to unused columns. Eigen Newton functors still need the full
        // present_fp set (functor_params), so keep full decls there.
        if (type == BlockType::State && state_kernel_uses_only_specialized_procedures() &&
            !info.eigen_newton_solver_exist && !info.eigen_linear_solver_exist) {
            print_present_fp_pointer_declarations_for(live_fp);
        } else {
            // General: all RANGE bases so procedure/functor args stay valid;
            // OpenACC present clause below is still live-set only (H4c).
            print_present_fp_pointer_declarations();
        }
        print_present_dptr_pointer_declarations_for(live_dptr);
    }

    std::ostringstream present_clause;
    present_clause << "present(_ml_arg, nt";
    if (type == BlockType::NetReceive) {
        // nrb metadata + Weight SoA + mechanism RANGE columns for NET_RECEIVE body.
        present_clause << ", nrb"
                          ", nrb->_displ[:nrb->_displ_cnt + 1]"
                          ", nrb->_nrb_index[:nrb->_cnt]"
                          ", nrb->_pnt_index[:nrb->_cnt]"
                          ", nrb->_weight_index[:nrb->_cnt]"
                          ", nrb->_nrb_t[:nrb->_cnt]"
                          ", nrb->_nrb_flag[:nrb->_cnt]"
                          ", weights[:weight_count]";
        if (info.net_send_used || info.net_event_used) {
            // Device net_send_buffering writes; host flush resolves indices after wait.
            present_clause << ", nsb"
                              ", nsb->_sendtype[:nsb->_size]"
                              ", nsb->_vdata_index[:nsb->_size]"
                              ", nsb->_weight_index[:nsb->_size]"
                              ", nsb->_pnt_index[:nsb->_size]"
                              ", nsb->_nsb_t[:nsb->_size]"
                              ", nsb->_nsb_flag[:nsb->_size]";
        }
        // NetReceive: present all float columns (buffer path may touch many).
        const auto codegen_float_variables_size = codegen_float_variables.size();
        for (int i = 0; i < codegen_float_variables_size; ++i) {
            const auto& float_var = codegen_float_variables[i];
            auto const array_len = float_var->is_array() ? float_var->get_length() : 1;
            present_clause << fmt::format(
                ", _present_fp_{}[:static_cast<std::size_t>(_ml_arg->nodecount) * {}]", i, array_len);
        }
    } else if (!info.artificial_cell) {
        present_clause << ", nodeindices";
        // Specialized STATE (force-inlined rates): no _thread / _ppvar use — drop
        // unused present(_thread) residual (hand-edit shape).
        const bool state_slim =
            type == BlockType::State && state_kernel_uses_only_specialized_procedures() &&
            !info.eigen_newton_solver_exist && !info.eigen_linear_solver_exist;
        if (!state_slim) {
            present_clause << ", _thread";
        }
        if (type == BlockType::Equation) {
            present_clause << ", vec_rhs[:nt->end], vec_d[:nt->end]";
        }
        std::vector<int> ordered_fp(live_fp.begin(), live_fp.end());
        std::sort(ordered_fp.begin(), ordered_fp.end());
        for (int i: ordered_fp) {
            const auto& float_var = codegen_float_variables[static_cast<size_t>(i)];
            auto const array_len = float_var->is_array() ? float_var->get_length() : 1;
            present_clause << fmt::format(
                ", _present_fp_{}[:static_cast<std::size_t>(_ml_arg->nodecount) * {}]", i, array_len);
        }
        // pdata ion rows live in device memory via upload_mechanism_pointer_tables;
        // do not add them to present() (OpenACC cannot track deviceptr slices here).
    }
    // Global struct is on data present; also list for kernels that touch TABLE.
    if (!info.artificial_cell && !codegen_global_variables.empty() && type != BlockType::NetReceive) {
        present_clause << ", " << global_struct_instance();
    }
    present_clause << ')';

    std::string deviceptr_extra = present_dptr_deviceptr_clause_for(live_dptr);
    if (!info.artificial_cell && type != BlockType::NetReceive) {
        if (deviceptr_extra.empty()) {
            deviceptr_extra = " deviceptr(_d_voltages)";
        } else {
            // present_dptr returns " deviceptr(a, b, ...)"; insert _d_voltages first.
            auto pos = deviceptr_extra.find("deviceptr(");
            if (pos != std::string::npos) {
                deviceptr_extra.insert(pos + std::strlen("deviceptr("), "_d_voltages, ");
            }
        }
    }

    // force_seq_acc_loop_: NRN_DETERMINISTIC_MATRIX PP path — serial instance order
    // matches CPU association for vec_rhs/vec_d (no unordered atomics).
    if (force_seq_acc_loop_) {
        printer->fmt_line(
            "nrn_pragma_acc(parallel loop seq {}{} async(nt->stream_id) if(nt->compute_gpu))",
            present_clause.str(),
            type == BlockType::NetReceive ? std::string{} : deviceptr_extra);
        printer->add_line(
            "nrn_pragma_omp(target teams distribute parallel for if(nt->compute_gpu))");
    } else {
        printer->fmt_line(
            "nrn_pragma_acc(parallel loop {}{} async(nt->stream_id) if(nt->compute_gpu))",
            present_clause.str(),
            type == BlockType::NetReceive ? std::string{} : deviceptr_extra);
        printer->add_line(
            "nrn_pragma_omp(target teams distribute parallel for if(nt->compute_gpu))");
    }
}

void CodegenNeuronAccVisitor::print_kernel_data_present_annotation_block_begin() {
    if (!info.artificial_cell) {
        if (codegen_global_variables.empty()) {
            printer->add_line("nrn_pragma_acc(data present(nt, _ml_arg) if(nt->compute_gpu))");
        } else {
            printer->fmt_line("nrn_pragma_acc(data present(nt, _ml_arg, {}) if(nt->compute_gpu))",
                              global_struct_instance());
        }
        printer->add_line("{");
        printer->increase_indent();
    }
}

void CodegenNeuronAccVisitor::print_kernel_data_present_annotation_block_end() {
    if (!info.artificial_cell) {
        print_device_stream_wait();
        printer->pop_block();
    }
}

void CodegenNeuronAccVisitor::print_device_stream_wait() const {
    printer->push_block("if(nt->compute_gpu)");
    printer->add_line("nrn_pragma_acc(wait(nt->stream_id))");
    printer->pop_block();
}

void CodegenNeuronAccVisitor::print_net_send_buffering_cnt_update() const {
    // Always atomic capture. Do not branch on nt->compute_gpu: the device copy of
    // NrnThread can have a stale compute_gpu==0 while the parallel region still
    // runs on the GPU (OpenACC evaluates the loop's if() on the host). A non-atomic
    // cnt++ then races and drops almost all net_send slots (Traub: ~300 of ~1e5).
    printer->add_line("nrn_pragma_acc(atomic capture)");
    printer->add_line("nrn_pragma_omp(atomic capture)");
    printer->add_line("i = nsb->_cnt++;");
}

void CodegenNeuronAccVisitor::print_net_send_buffering_grow() {
    // No-op for ACC: OpenACC device-compiles this whole function when it is called
    // from a parallel loop, so host-only ensure/grow symbols must not appear here
    // (nvlink undefined reference). Host must pre-size before the OpenACC region
    // (net_send_buffer_ensure_for_events: headroom × events + high-water). If
    // i >= nsb->_size the slot is not written; update_net_send_buffer_on_host
    // aborts when _cnt > _size (never silent drop).
    (void) 0;
}

void CodegenNeuronAccVisitor::print_net_send_buf_count_update_to_host() const {
    printer->add_line("nrn_pragma_acc(update self(nsb->_cnt))");
    printer->add_line("nrn_pragma_omp(target update from(nsb->_cnt))");
}

void CodegenNeuronAccVisitor::print_net_send_buf_update_to_host() const {
    print_device_stream_wait();
    printer->push_block("if (nsb && nt->compute_gpu)");
    print_net_send_buf_count_update_to_host();
    printer->add_line("neuron::gpu::update_net_send_buffer_on_host(nt, nsb);");
    printer->pop_block();
}

void CodegenNeuronAccVisitor::print_net_send_buf_count_update_to_device() const {
    printer->push_block("if (nt->compute_gpu)");
    printer->add_line("nrn_pragma_acc(update device(nsb->_cnt))");
    printer->add_line("nrn_pragma_omp(target update to(nsb->_cnt))");
    printer->pop_block();
}

void CodegenNeuronAccVisitor::print_net_send_buffering() {
    if (!net_send_buffer_required()) {
        return;
    }

    printer->add_newline(2);
    // Index ABI: tqitem ppvar field, weight SoA index, mechanism instance id (not host pointers).
    // Device-callable: no host-only ensure/grow (pre-size on host before OpenACC regions).
    auto args =
        "NrnThread* nt, Memb_list* ml, neuron::gpu::NetSendBuffer_t* nsb, int type, "
        "int vdata_index, int weight_index, int pnt_instance_id, double t, double flag";
    printer->fmt_push_block("static inline void net_send_buffering({})", args);
    printer->add_line("(void) ml;");
    printer->add_line("int i = 0;");
    print_net_send_buffering_cnt_update();
    printer->push_block("if (i < nsb->_size)");
    printer->add_multi_line(R"CODE(
         nsb->_sendtype[i] = type;
         nsb->_vdata_index[i] = vdata_index;
         nsb->_weight_index[i] = weight_index;
         nsb->_pnt_index[i] = pnt_instance_id;
         nsb->_nsb_t[i] = t;
         nsb->_nsb_flag[i] = flag;
    )CODE");
    printer->pop_block();
    printer->pop_block();
}

void CodegenNeuronAccVisitor::print_send_event_move() {
    printer->add_newline();
    printer->add_line("neuron::gpu::NetSendBuffer_t* nsb = _ml_arg->_net_send_buffer;");
    print_net_send_buf_update_to_host();
    printer->add_line("neuron::gpu::deliver_net_send_buffer_events(nt, _ml_arg, nsb);");
    print_net_send_buf_count_update_to_device();
}

void CodegenNeuronAccVisitor::print_after_nrn_cur_gpu_net_send_flush() {
    if (info.net_send_used && !info.artificial_cell) {
        print_send_event_move();
    }
}

void CodegenNeuronAccVisitor::print_compute_functions() {
    print_net_send_buffering();
    print_nrn_init();
    print_nrn_cur();
    print_nrn_state();
    print_nrn_jacob();
    // Host WATCH cond/alloc before NET_RECEIVE (WATCH mechs use host receive).
    print_watch_support();
    // Stage 2: device NET_RECEIVE kernel + buffering before host pnt_receive (rename once).
    print_net_receive_buffering();
    print_net_receive();
    print_net_init();
}

void CodegenNeuronAccVisitor::print_nrn_jacob() {
    if (!breakpoint_exist() || info.artificial_cell) {
        CodegenNeuronCppVisitor::print_nrn_jacob();
        return;
    }

    printer->add_newline(2);

    ParamVector args = {{"", "const _nrn_model_sorted_token&", "", "_sorted_token"},
                        {"", "NrnThread*", "", "nt"},
                        {"", "Memb_list*", "", "_ml_arg"},
                        {"", "int", "", "_type"}};

    printer->fmt_push_block("static void {}({})",
                            method_name(naming::NRN_JACOB_METHOD),
                            get_parameter_str(args));
    // Gate on matrix residency (Gate A), not the global Gate-B aggregate: nrn_cur already
    // wrote g_unused on device; host jacob would read stale host SOA and skip vec_d updates.
    printer->push_block(
        "if (nt->compute_gpu && neuron::gpu::matrix_rhs_d_stays_on_device_for_solve(*nt))");
    print_kernel_global_device_setup();
    print_entrypoint_setup_code_from_memb_list();
    printer->fmt_line("auto nodecount = _ml_arg->nodecount;");
    use_present_fp_indexing_ = true;
    auto print_jacob_device_loop = [&](bool det_matrix) {
        force_seq_acc_loop_ = det_matrix;
        // Jacob only touches g_unused / conductance column + vec_d.
        live_float_indices_override_ = std::unordered_set<int>{conductance_fp_index()};
        print_parallel_iteration_hint(BlockType::Equation, nullptr);
        live_float_indices_override_.reset();
        printer->push_block("for (int id = 0; id < nodecount; id++)");
        printer->add_line("int node_id = node_data.nodeindices[id];");
        // g_unused from nrn_cur. PP: atomic unless NRN_DETERMINISTIC_MATRIX (seq loop).
        if (info.point_process && !det_matrix) {
            printer->add_line("nrn_pragma_acc(atomic update)");
            printer->add_line("nrn_pragma_omp(atomic update)");
        }
        // _present_fp_* from fpfield_ptr() already include ml storage offset.
        printer->fmt_line("vec_d[node_id] {} _present_fp_{}[id];",
                          operator_for_d(),
                          conductance_fp_index());
        printer->pop_block();
        force_seq_acc_loop_ = false;
    };
    if (info.point_process) {
        printer->push_block("if (neuron::event_order::matrix_enabled())");
        print_jacob_device_loop(true);
        printer->chain_block("else");
        print_jacob_device_loop(false);
        printer->pop_block();
    } else {
        print_jacob_device_loop(false);
    }
    use_present_fp_indexing_ = false;
    print_device_stream_wait();
    printer->chain_block("else");
    print_entrypoint_setup_code_from_memb_list();
    printer->fmt_line("auto nodecount = _ml_arg->nodecount;");
    printer->push_block("for (int id = 0; id < nodecount; id++)");
    printer->add_line("int node_id = node_data.nodeindices[id];");
    printer->fmt_line("node_data.node_diagonal[node_id] {} _lmc.template fpfield<{}>(id);",
                      operator_for_d(),
                      conductance_fp_index());
    if (info.electrode_current) {
        printer->push_block("if (auto* vec_sav_d = nt->node_sav_d_storage())");
        printer->fmt_line("vec_sav_d[node_id] {} _lmc.template fpfield<{}>(id);",
                          operator_for_d(),
                          conductance_fp_index());
        printer->pop_block();
    }
    printer->pop_block();
    printer->pop_block();
    printer->pop_block();
}

void CodegenNeuronAccVisitor::print_check_table_entrypoint() {
    if (info.table_count == 0) {
        return;
    }

    for (const auto& function: info.functions_with_table) {
        auto name = function->get_node_name();
        auto internal_params = internal_method_parameters();
        printer->fmt_line("void {}({});",
                          table_update_function_name(name),
                          get_parameter_str(internal_params));
    }

    ParamVector args = {{"", "Memb_list*", "", "_ml"},
                        {"", "size_t", "", "id"},
                        {"", "Datum*", "", "_ppvar"},
                        {"", "Datum*", "", "_thread"},
                        {"", "double*", "", "_globals"},
                        {"", "NrnThread*", "", "nt"},
                        {"", "int", "", "_type"},
                        {"", "const _nrn_model_sorted_token&", "", "_sorted_token"}};

    printer->fmt_line("static void {}({})", table_thread_function_name(), get_parameter_str(args));
    printer->push_block();
    printer->add_line("_nrn_mechanism_cache_range _lmc{_sorted_token, *nt, *_ml, _type};");
    print_present_fp_pointer_declarations();
    printer->fmt_line("auto inst = make_instance_{}(_ml->get_storage_offset() + id, &_lmc, false);",
                      info.mod_suffix);
    if (!info.artificial_cell) {
        printer->fmt_line("auto node_data = make_node_data_{}(*nt, *_ml);", info.mod_suffix);
    }
    if (!codegen_thread_variables.empty()) {
        printer->fmt_line("auto _thread_vars = {}(_thread[{}].get<double*>());",
                          thread_variables_struct(),
                          info.thread_var_thread_id);
    }

    // Table thread check is host glue: bind TABLE temps from SoA elements.
    hoc_wrapper_table_temp_as_soa_ = true;
    for (const auto& function: info.functions_with_table) {
        auto method_name = function->get_node_name();
        printer->fmt_line("{}({});",
                          table_update_function_name(method_name),
                          internal_method_arguments());
    }
    hoc_wrapper_table_temp_as_soa_ = false;
    printer->pop_block();
}

void CodegenNeuronAccVisitor::print_gpu_phase_registration() {
    std::vector<std::string> flags;
    if (nrn_cur_required()) {
        flags.emplace_back("neuron::gpu::MechanismGpuPhase::Current");
    }
    if (breakpoint_exist()) {
        flags.emplace_back("neuron::gpu::MechanismGpuPhase::Jacobian");
    }
    if (nrn_state_required()) {
        flags.emplace_back("neuron::gpu::MechanismGpuPhase::Solve");
    }
    if (flags.empty()) {
        return;
    }
    std::string phase_flags;
    for (std::size_t i = 0; i < flags.size(); ++i) {
        if (i > 0) {
            phase_flags += " | ";
        }
        phase_flags += flags[i];
    }
    printer->fmt_line("neuron::gpu::register_mechanism_gpu_phases(mech_type, {});", phase_flags);
}

void CodegenNeuronAccVisitor::print_net_receive_registration() {
    CodegenNeuronCppVisitor::print_net_receive_registration();
    if (net_receive_buffering_required()) {
        printer->fmt_line("hoc_register_net_receive_buffering(net_buf_receive_{}, mech_type);",
                          info.mod_suffix);
    }
}

void CodegenNeuronAccVisitor::print_net_receive() {
    auto node = info.net_receive_node;
    if (!node) {
        return;
    }

    printing_net_receive = true;
    // Rename NET_RECEIVE args → _args[i] once for host + device bodies.
    rename_net_receive_arguments(*node, *node);

    printer->fmt_push_block("static void nrn_net_receive_{}({})",
                            info.mod_suffix,
                            get_parameter_str(net_receive_args()));

    // Always resolve thread once (shared by GPU enqueue and host path).
    printer->add_line("auto* nt = static_cast<NrnThread*>(_pnt->_vnt);");

    // Stage 2: device net_buf_receive for ordinary NET_RECEIVE. Host path when:
    // - WATCH (host WatchCondition + RANGE write for device CURRENT)
    // - BBCOREPOINTER (Gfluct3 oup/mynormrand: host Random123; push SoA after)
    const bool host_net_receive =
        info.is_watch_used() || info.bbcore_pointer_used;
    if (net_receive_buffering_required() && !host_net_receive) {
        printer->push_block("if (nt && nt->compute_gpu)");
        printer->add_line(
            "Memb_list* ml = nt->_ml_list[_nrn_mechanism_get_type(_pnt->prop)];");
        printer->push_block("if (ml)");
        printer->add_line(
            "int pnt_index = static_cast<int>(neuron::mechanism::_get::_current_row(_pnt->prop) - "
            "ml->get_storage_offset());");
        printer->add_line("neuron::gpu::net_receive_buffer_ensure(ml);");
        printer->add_line(
            "neuron::gpu::net_receive_buffer_enqueue(nt, ml, pnt_index, _weight_index, flag);");
        printer->pop_block();
        printer->add_line("return;");
        printer->pop_block();
    }

    // Host path (CPU backend, non-GPU threads, or WATCH mechs under native GPU).
    // (nt already declared above — do not redeclare.)
    printer->add_line("_nrn_mechanism_cache_instance _lmc{_pnt->prop};");
    printer->add_line("auto * _ppvar = _nrn_mechanism_access_dparam(_pnt->prop);");
    printer->fmt_line("double* _args = _nrn_netrec_wsoa(_weight_index, {});",
                      info.num_net_receive_parameters);
    printer->fmt_line("auto inst = make_instance_{}(&_lmc);", info.mod_suffix);
    if (!info.artificial_cell) {
        printer->fmt_line("auto node_data = make_node_data_{}(_pnt->prop);", info.mod_suffix);
    }
    printer->add_line("Datum * _thread = nullptr;");
    printer->add_line("size_t id = 0;");
    printer->add_line("double t = nt->_t;");
    // MOD FUNCTION/PROCEDURE signatures always take _present_fp_* (ACC). Host
    // NET_RECEIVE body must declare them or Gfluct3-style calls fail to compile.
    print_present_fp_pointer_declarations();
    if (info.is_watch_used()) {
        printer->add_line("int _watch_rm = 0;");
    }
    // Reset watch index counter so activates match _watchN_cond numbering.
    current_watch_statement = 0;
    print_statement_block(*node->get_statement_block(), false, false);
    printer->fmt_line("_nrn_netrec_wsoa_done(_weight_index, {}, _args);",
                      info.num_net_receive_parameters);
    if (host_net_receive) {
        // Device CURRENT (and BA fold) need host-written RANGE (g,e / g_e1,g_i1).
        printer->push_block("if (nt && nt->compute_gpu)");
        printer->add_line(
            "neuron::gpu::upload_present_mechanism_soa_to_device(_nrn_mechanism_get_type(_pnt->prop));");
        printer->pop_block();
    }
    printer->add_newline();
    printer->pop_block();
    printing_net_receive = false;
}

void CodegenNeuronAccVisitor::print_net_receive_buffering() {
    if (!net_receive_buffering_required()) {
        return;
    }

    auto* node = info.net_receive_node;
    if (!node) {
        return;
    }

    // WATCH / BBCOREPOINTER use host NET_RECEIVE only. Emitting an ACC kernel that
    // calls VERBATIM RANDOM (Gfluct3 mynormrand) fails device compile even if the
    // kernel is never entered — stub the device buffer path.
    if (info.is_watch_used() || info.bbcore_pointer_used) {
        printer->add_newline(2);
        printer->fmt_push_block("static void net_buf_receive_{}(NrnThread* /*nt*/)",
                                info.mod_suffix);
        printer->add_line("/* host-only NET_RECEIVE (WATCH or BBCOREPOINTER RANDOM) */");
        printer->pop_block();
        return;
    }

    // Ensure args are renamed before generating host + device bodies (mutates AST).
    rename_net_receive_arguments(*node, *node);

    printer->add_newline(2);
    printer->add_line(
        "/** CoreNEURON-style: apply queued NET_RECEIVE on device (Weight SoA + net_send buffer). */");
    printer->fmt_push_block("static void net_buf_receive_{}(NrnThread* nt)", info.mod_suffix);

    printer->push_block("if (!nt || !nt->compute_gpu)");
    printer->add_line("return;");
    printer->pop_block();

    printer->add_line("Memb_list* _ml_arg = nt->_ml_list[mech_type];");
    printer->push_block("if (!_ml_arg)");
    printer->add_line("return;");
    printer->pop_block();

    printer->add_line("neuron::gpu::NetReceiveBuffer_t* nrb = _ml_arg->_net_receive_buffer;");
    printer->push_block("if (!nrb || !nrb->_cnt)");
    printer->add_line("return;");
    printer->pop_block();

    printer->add_line("auto const& _sorted_token = nrn_ensure_model_data_are_sorted();");
    printer->add_line(
        "_nrn_mechanism_cache_range _lmc{_sorted_token, *nt, *_ml_arg, _ml_arg->type()};");
    printer->fmt_line(
        "auto inst = make_instance_{}(_ml_arg->get_storage_offset(), &_lmc, /*use_device_ptrs*/ true);",
        info.mod_suffix);
    if (!info.artificial_cell) {
        // Needed when NET_RECEIVE body calls MOD FUNCTIONs (args include node_data).
        printer->fmt_line(
            "auto node_data = make_node_data_{}(*nt, *_ml_arg, /*use_device_ptrs*/ true);",
            info.mod_suffix);
    }
    printer->add_line("auto* _thread = _ml_arg->_thread;");
    if (!codegen_thread_variables.empty()) {
        printer->fmt_line("auto _thread_vars = {}(_thread[{}].get<double*>());",
                          thread_variables_struct(),
                          info.thread_var_thread_id);
    }

    const auto codegen_float_variables_size = codegen_float_variables.size();
    for (int i = 0; i < codegen_float_variables_size; ++i) {
        printer->fmt_line("double* _present_fp_{0} = _lmc.template fpfield_ptr<{0}>();", i);
    }
    // POINTER/RANDOM dptrs for FUNCTION bodies called from NET_RECEIVE (e.g. Gfluct3).
    print_present_dptr_pointer_declarations();

    printer->add_line("double* weights = neuron::gpu::weight_soa_values();");
    printer->add_line("std::size_t weight_count = neuron::gpu::weight_soa_count();");
    printer->push_block("if (!weights || weight_count == 0)");
    printer->add_line("return;");
    printer->pop_block();

    if (info.net_send_used || info.net_event_used) {
        // Pre-size for this flush: min_events × headroom (default 4) + high-water.
        // Device cannot grow mid-kernel; overflow aborts on host flush.
        printer->add_line(
            "neuron::gpu::net_send_buffer_ensure_for_events(_ml_arg, nrb->_cnt);");
    }

    // Parallel over unique instances; serial over events per instance (CoreNEURON).
    // Body uses _present_fp_* + weights[weight_index+arg]; net_send → NetSendBuffer.
    printer->add_line("int count = nrb->_displ_cnt;");
    print_kernel_global_device_setup();
    // Keep device nt->_t fresh for any residual uses; net_send absolute times use
    // per-event local `t` (nrb->_nrb_t) — see get_variable_name("t").
    printer->push_block("if (nt->compute_gpu)");
    printer->add_line("nrn_pragma_acc(update device(nt->_t))");
    printer->add_line("nrn_pragma_omp(target update to(nt->_t))");
    printer->pop_block();
    // Scope nsb for present() so print_send_event_move can redeclare after.
    printer->add_line("{");
    printer->increase_indent();
    if (info.net_send_used || info.net_event_used) {
        printer->add_line("neuron::gpu::NetSendBuffer_t* nsb = _ml_arg->_net_send_buffer;");
        printer->push_block("if (!nsb)");
        printer->add_line("return;");
        printer->pop_block();
    }
    print_kernel_data_present_annotation_block_begin();
    use_present_fp_indexing_ = true;
    printing_net_buf_receive_kernel_ = true;
    print_parallel_iteration_hint(BlockType::NetReceive, node);
    printer->push_block("for (int i = 0; i < count; i++)");
    printer->add_line("int start = nrb->_displ[i];");
    printer->add_line("int end = nrb->_displ[i + 1];");
    printer->push_block("for (int j = start; j < end; j++)");
    printer->add_multi_line(R"CODE(
        int index = nrb->_nrb_index[j];
        int id = nrb->_pnt_index[index];
        double t = nrb->_nrb_t[index];  // event delivery time (not nt->_t after restore)
        int weight_index = nrb->_weight_index[index];
        double flag = nrb->_nrb_flag[index];
        double* _args = weights + weight_index;
        Datum* _ppvar = _ml_arg->pdata ? _ml_arg->pdata[id] : nullptr;
    )CODE");
    printing_net_receive = true;
    print_statement_block(*node->get_statement_block(), false, false);
    printing_net_receive = false;
    printer->pop_block();  // inner j
    printer->pop_block();  // outer i
    printing_net_buf_receive_kernel_ = false;
    use_present_fp_indexing_ = false;
    print_kernel_data_present_annotation_block_end();  // wait(stream)
    printer->decrease_indent();
    printer->add_line("}");

    printer->add_line("nrb->_displ_cnt = 0;");
    printer->add_line("nrb->_cnt = 0;");
    printer->push_block("if (nt->compute_gpu && nrb->device_uploaded)");
    printer->add_line("nrn_pragma_acc(update device(nrb->_cnt, nrb->_displ_cnt))");
    printer->add_line("nrn_pragma_omp(target update to(nrb->_cnt, nrb->_displ_cnt))");
    printer->pop_block();

    if (info.net_send_used || info.net_event_used) {
        print_send_event_move();
    }

    printer->pop_block();  // net_buf_receive
}

void CodegenNeuronAccVisitor::print_net_send_call(const ast::FunctionCall& node) {
    // Host pnt_receive path, or host-only INITIAL (no present_fp / no nsb):
    // direct net_send (CPU queue API).
    if (((printing_net_receive || printing_net_init) && !printing_net_buf_receive_kernel_) ||
        !use_present_fp_indexing_) {
        CodegenNeuronCppVisitor::print_net_send_call(node);
        return;
    }

    auto const& arguments = node.get_arguments();
    if (info.artificial_cell) {
        const auto& tqitem = get_variable_name("tqitem", /* use_instance */ false);
        std::string point_process = get_variable_name(naming::POINT_PROCESS_VARIABLE, false);
        if (!printing_net_receive) {
            point_process = fmt::format("(Point_process*){}", point_process);
        }
        std::string weight_index = printing_net_receive ? "weight_index" : "-1";
        printer->fmt_text("{}(/* tqitem */ &{}, {}, {}, {} + ",
                          "artcell_net_send",
                          tqitem,
                          weight_index,
                          point_process,
                          get_variable_name("t"));
        print_vector_elements(arguments, ", ");
        printer->add_text(')');
        return;
    }

    // Device path: buffer indices; host deliver resolves via Memb_list pdata.
    // Use present-mapped local `nsb` (same object as update self(nsb->_cnt)).
    int const tq_field = info.tqitem_index >= 0 ? info.tqitem_index : -1;
    std::string const weight_index = printing_net_receive ? "weight_index" : "-1";
    const auto& t = get_variable_name("t");
    printer->add_text("net_send_buffering(");
    printer->fmt_text(
        "nt, _ml_arg, nsb, 0, {}, {}, id, {}+",
        tq_field,
        weight_index,
        t);
    print_vector_elements(arguments, ", ");
    printer->add_text(')');
}

void CodegenNeuronAccVisitor::print_net_move_call(const ast::FunctionCall& node) {
    if (!printing_net_receive && !printing_net_init) {
        throw std::runtime_error("Error : net_move only allowed in NET_RECEIVE block");
    }
    if ((printing_net_receive || printing_net_init) && !printing_net_buf_receive_kernel_) {
        CodegenNeuronCppVisitor::print_net_move_call(node);
        return;
    }

    if (info.artificial_cell) {
        const auto& tqitem = get_variable_name("tqitem", false);
        const auto& point_process = get_variable_name(naming::POINT_PROCESS_VARIABLE, false);
        printer->fmt_text("artcell_net_move(&{}, {}, ", tqitem, point_process);
        print_vector_elements(node.get_arguments(), ", ");
        printer->add_text(")");
        return;
    }

    int const tq_field = info.tqitem_index >= 0 ? info.tqitem_index : -1;
    printer->add_text("net_send_buffering(");
    printer->fmt_text(
        "nt, _ml_arg, nsb, 2, {}, -1, id, ",
        tq_field);
    print_vector_elements(node.get_arguments(), ", ");
    printer->add_text(", 0.0");
    printer->add_text(")");
}

void CodegenNeuronAccVisitor::print_net_event_call(const ast::FunctionCall& node) {
    const auto& arguments = node.get_arguments();
    if (info.artificial_cell) {
        // Host NET_RECEIVE param is `_pnt` (see net_receive_args).
        printer->add_text("net_event(_pnt, ");
        print_vector_elements(arguments, ", ");
        printer->add_text(")");
        return;
    }
    if (printing_net_buf_receive_kernel_ || (!printing_net_receive && !printing_net_init)) {
        printer->add_text("net_send_buffering(");
        // present-mapped local nsb — see print_net_send_call.
        printer->add_text(
            "nt, _ml_arg, nsb, 1, -1, -1, id, ");
        print_vector_elements(arguments, ", ");
        printer->add_text(", 0.0");
        printer->add_text(")");
        return;
    }
    // Host pnt_receive
    const auto& point_process = get_variable_name(naming::POINT_PROCESS_VARIABLE, false);
    printer->fmt_text("net_event({}, t)", point_process);
}

void CodegenNeuronAccVisitor::print_mechanism_range_var_structure(bool print_initializers) {
    auto const value_initialize = print_initializers ? "{}" : "";
    printer->add_newline(2);
    printer->add_line("/** mechanism instance: SoA index base + ion pdata handles (GPU index path) */");
    printer->fmt_push_block("struct {} ", instance_struct());

    for (auto const& [var, type]: info.neuron_global_variables) {
        auto const name = var->get_name();
        printer->fmt_line("{}* {}{};",
                          type,
                          name,
                          print_initializers ? fmt::format("{{&::{}}}", name) : std::string{});
    }

    printer->fmt_line("std::size_t _data_offset{};", value_initialize);

    for (auto& var: codegen_int_variables) {
        const auto& name = var.symbol->get_name();
        auto position = position_of_int_var(name);

        if (name == naming::POINT_PROCESS_VARIABLE) {
            continue;
        } else if (var.is_index || var.is_integer) {
        } else if (info.semantics[position].name == naming::POINTER_SEMANTIC) {
            // Host POINTER; not device-present via Instance.
        } else if (info.semantics[position].name == naming::RANDOM_SEMANTIC || var.is_vdata) {
            // RANDOM / vdata use _ppvar on host (and device NET_RECEIVE); not in
            // Instance SoA — including them mis-orders make_instance args.
        } else {
            auto qualifier = var.is_constant ? "const " : "";
            auto type = var.is_vdata ? "void*" : default_float_data_type();
            printer->fmt_line("{}{}* const* {}{};", qualifier, type, name, value_initialize);
        }
    }

    if (!codegen_global_variables.empty()) {
        printer->fmt_line("{}* {}{};",
                          global_struct(),
                          naming::INST_GLOBAL_MEMBER,
                          print_initializers ? fmt::format("{{&{}}}", global_struct_instance())
                                             : std::string{});
    }
    printer->pop_block(";");
}

std::string CodegenNeuronAccVisitor::indexed_fp_var(std::string_view name,
                                                    std::string_view index_expr) const {
    auto const position = position_of_float_var(std::string{name});
    if (!use_present_fp_indexing_) {
        // Host-only paths (e.g. INITIAL with wrote_conc) never declare _present_fp_*.
        return fmt::format("_lmc.template fpfield<{}>({})", position, index_expr);
    }
    // present_fp pointers are already advanced by ml storage offset (see
    // MechanismRange::fpfield_ptr). Index with local id only — adding
    // inst._data_offset again double-counts and SEGVs on multi-thread.
    return fmt::format("_present_fp_{}[{}]", position, index_expr);
}

int CodegenNeuronAccVisitor::conductance_fp_index() const {
    auto const& name = info.vectorize ? naming::CONDUCTANCE_UNUSED_VARIABLE
                                      : naming::CONDUCTANCE_VARIABLE;
    return position_of_float_var(name);
}

void CodegenNeuronAccVisitor::print_v_unused() const {
    if (!info.vectorize) {
        return;
    }
    printer->fmt_line("{} = v;", indexed_fp_var(naming::VOLTAGE_UNUSED_VARIABLE));
}

void CodegenNeuronAccVisitor::print_g_unused() const {
    printer->add_multi_line(R"CODE(
        #if NRN_PRCELLSTATE
    )CODE");
    printer->fmt_line("{} = g;", indexed_fp_var(naming::CONDUCTANCE_UNUSED_VARIABLE));
    printer->add_line("#endif");
}

void CodegenNeuronAccVisitor::print_nrn_init(bool skip_init_check) {
    (void) skip_init_check;
    printer->add_newline(2);

    print_global_function_common_code(BlockType::Initial);

    // Host-only Initial (e.g. ion wrote_conc) falls back to CPU ivdep path and
    // never declares _present_fp_* / _present_dptr_*. Must not use present_fp
    // indexing in that body or cad/ion WRITE INITIAL fails to compile.
    const bool host_only_init = host_only_parallel_block(BlockType::Initial);
    use_present_fp_indexing_ = !host_only_init;
    print_parallel_iteration_hint(BlockType::Initial, info.initial_node);
    printer->push_block("for (int id = 0; id < nodecount; id++)");

    printer->add_line("auto* _ppvar = _ml_arg->pdata[id];");
    if (!info.artificial_cell) {
        printer->add_line("int node_id = node_data.nodeindices[id];");
        // Host-only INITIAL (wrote_conc) uses the CPU ivdep path and never
        // declares _d_voltages / present_fp_* — read host node_voltages there.
        if (host_only_init) {
            printer->fmt_line("{} = node_data.node_voltages[node_id];",
                              indexed_fp_var(naming::VOLTAGE_UNUSED_VARIABLE));
        } else {
            printer->fmt_line("{} = _d_voltages[node_id];",
                              indexed_fp_var(naming::VOLTAGE_UNUSED_VARIABLE));
        }
    }

    print_rename_state_vars();

    if (!info.changed_dt.empty()) {
        printer->fmt_line("double _save_prev_dt = {};",
                          get_variable_name(naming::NTHREAD_DT_VARIABLE));
        printer->fmt_line("{} = {};",
                          get_variable_name(naming::NTHREAD_DT_VARIABLE),
                          info.changed_dt);
    }

    print_initial_block(info.initial_node);

    if (!info.changed_dt.empty()) {
        printer->fmt_line("{} = _save_prev_dt;", get_variable_name(naming::NTHREAD_DT_VARIABLE));
    }

    printer->pop_block();
    print_kernel_data_present_annotation_block_end();
    printer->pop_block();
    use_present_fp_indexing_ = false;
}

void CodegenNeuronAccVisitor::print_before_breakpoint_inline() {
    // NEURON ACC does not emit separate hoc_reg_ba for BEFORE BREAKPOINT yet.
    // Fold into CURRENT so ival/g updates (Gfluct3) run on device after host
    // NET_RECEIVE has pushed OU state (g_e1/g_i1).
    for (const auto* block: info.before_after_blocks) {
        if (!block || !block->is_before_block()) {
            continue;
        }
        auto ba_block =
            dynamic_cast<const ast::BeforeBlock*>(block)->get_bablock();
        if (!ba_block || ba_block->get_type()->get_value() != ast::BATYPE_BREAKPOINT) {
            continue;
        }
        print_statement_block(*ba_block->get_statement_block(), false, false);
    }
}

void CodegenNeuronAccVisitor::print_nrn_current(const ast::BreakpointBlock& node) {
    use_present_fp_indexing_ = true;
    // Body reads t via get_variable_name → _nrn_thread_t parameter (firstprivate
    // from host). Device nt->_t is not reliable during CURRENT (IClamp window).
    use_host_captured_t_ = true;
    const auto& args = nrn_current_parameters();
    const auto& block = node.get_statement_block();
    printer->add_newline(2);
    printer->fmt_push_block("static inline double nrn_current_{}({})",
                            info.mod_suffix,
                            get_parameter_str(args));
    printer->fmt_line("{} = v;", indexed_fp_var(naming::VOLTAGE_UNUSED_VARIABLE));
    printer->add_line("double current = 0.0;");
    print_before_breakpoint_inline();
    print_statement_block(*block, false, false);
    for (auto& current: info.currents) {
        const auto& name = get_variable_name(current);
        printer->fmt_line("current += {};", name);
    }
    printer->add_line("return current;");
    printer->pop_block();
    use_present_fp_indexing_ = false;
    use_host_captured_t_ = false;
}

void CodegenNeuronAccVisitor::print_nrn_state() {
    if (!nrn_state_required()) {
        return;
    }

    printer->add_newline(2);
    print_global_function_common_code(BlockType::State);
    if ((info.net_send_used || info.net_event_used) && !info.artificial_cell) {
        printer->add_line(
            "neuron::gpu::net_send_buffer_ensure_for_events(_ml_arg, nodecount);");
    }
    // Host-captured t for any STATE body that reads t (same as nrn_cur).
    printer->add_line("double const _nrn_thread_t = nt->_t;");
    printer->add_line("(void) _nrn_thread_t;");  // unused when STATE does not reference t
    use_host_captured_t_ = true;

    use_present_fp_indexing_ = true;
    print_parallel_iteration_hint(BlockType::State, info.nrn_state_block);
    printer->push_block("for (int id = 0; id < nodecount; id++)");
    // Prefer local nodeindices (present) over node_data.nodeindices (hand-edit).
    if (!info.artificial_cell) {
        printer->add_line("int node_id = nodeindices[id];");
    } else {
        printer->add_line("int node_id = node_data.nodeindices[id];");
    }
    // Skip unused _ppvar when STATE only force-inlines specialized procedures.
    if (!(state_kernel_uses_only_specialized_procedures() && !info.eigen_newton_solver_exist &&
          !info.eigen_linear_solver_exist)) {
        printer->add_line("auto* _ppvar = _ml_arg->pdata[id];");
    }

    // H4c: stack locals for TABLE rates temps (minf/mtau/…) — no hot-path SoA.
    if (!info.table_statement_variables.empty()) {
        for (const auto& v: info.table_statement_variables) {
            if (v && !v->is_array()) {
                printer->fmt_line("double {};", v->get_name());
            } else if (v && v->is_array()) {
                printer->fmt_line("double {}[{}];", v->get_name(), v->get_length());
            }
        }
        state_kernel_locals_active_ = true;
    }

    // H4c: local v (not v_unused SoA) — rates(v) and no voltage present.
    if (!info.artificial_cell) {
        printer->add_line("double v = _d_voltages[node_id];");
        state_local_v_active_ = true;
    }

    if (ion_variable_struct_required()) {
        throw std::runtime_error("Not implemented.");
    }

    // H4b: emit ion READ only if the shadow var is used on the STATE path
    // (nrn_state block + procedures/functions STATE may call). HH rates(v)
    // does not use ena/ek — those loads were dead (CURRENT still reads them).
    {
        std::unordered_set<std::string> used_names;
        auto collect_names = [&](const ast::Ast& node) {
            for (const auto& n: collect_nodes(node, {ast::AstNodeType::NAME})) {
                auto nm = n->get_node_name();
                if (!nm.empty()) {
                    used_names.insert(std::move(nm));
                }
            }
        };
        if (info.nrn_state_block) {
            collect_names(*info.nrn_state_block);
        }
        for (const auto& procedure: info.procedures) {
            if (procedure) {
                collect_names(*procedure);
            }
        }
        for (const auto& function: info.functions) {
            if (function) {
                collect_names(*function);
            }
        }
        for (const auto& ion: info.ions) {
            for (const auto& var: ion.reads) {
                auto const iter =
                    std::find(ion.implicit_reads.begin(), ion.implicit_reads.end(), var);
                if (iter != ion.implicit_reads.end() || used_names.count(var) == 0) {
                    continue;
                }
                auto variable_names = read_ion_variable_name(var);
                printer->fmt_line("{} = {};",
                                  get_variable_name(variable_names.first),
                                  get_variable_name(variable_names.second));
            }
            for (const auto& var: ion.writes) {
                if (!ion.is_ionic_conc(var) || used_names.count(var) == 0) {
                    continue;
                }
                auto variables = read_ion_variable_name(var);
                printer->fmt_line("{} = {};",
                                  get_variable_name(variables.first),
                                  get_variable_name(variables.second));
            }
        }
    }

    if (info.nrn_state_block) {
        info.nrn_state_block->visit_children(*this);
    }

    for (auto& block: info.matexp_blocks) {
        if (!block->get_steadystate().get()->eval()) {
            block->accept(*this);
        }
    }

    if (info.currents.empty() && info.breakpoint_node != nullptr) {
        auto block = info.breakpoint_node->get_statement_block();
        print_statement_block(*block, false, false);
    }

    const auto& write_statements = ion_write_statements(BlockType::State);
    for (auto& statement: write_statements) {
        const auto& text = process_shadow_update_statement(statement, BlockType::State);
        printer->add_line(text);
    }

    printer->pop_block();
    state_kernel_locals_active_ = false;
    state_local_v_active_ = false;
    print_kernel_data_present_annotation_block_end();
    printer->pop_block();
    use_present_fp_indexing_ = false;
    use_host_captured_t_ = false;
}

CodegenNeuronAccVisitor::ParamVector CodegenNeuronAccVisitor::nrn_current_parameters() {
    auto params = CodegenNeuronCppVisitor::nrn_current_parameters();
    auto const v_param = params.back();
    params.pop_back();
    for (int i = 0; i < static_cast<int>(codegen_float_variables.size()); ++i) {
        params.emplace_back("", "double*", "", fmt::format("_present_fp_{}", i));
    }
    // Host-captured sim time (see get_variable_name for t under present_fp indexing).
    params.emplace_back("", "double", "", "_nrn_thread_t");
    params.push_back(v_param);
    return params;
}

void CodegenNeuronAccVisitor::print_nrn_cur_kernel(const ast::BreakpointBlock& node) {
    // Use _d_voltages (deviceptr), not present(host node_voltages).
    printer->add_line("int node_id = node_data.nodeindices[id];");
    printer->add_line("double v = _d_voltages[node_id];");
    printer->add_line("auto* _ppvar = _ml_arg->pdata[id];");
    const auto& read_statements = ion_read_statements(BlockType::Equation);
    for (auto& statement: read_statements) {
        printer->add_line(statement);
    }

    // Conductance path does not call nrn_current_*; fold BEFORE BREAKPOINT here.
    // Caller already set use_present_fp_indexing_ for the ACC CURRENT loop.
    // Mirror nrn_current_*: store local v into voltage SoA then run BA (uses v).
    if (!info.conductances.empty()) {
        printer->fmt_line("{} = v;", indexed_fp_var(naming::VOLTAGE_UNUSED_VARIABLE));
        print_before_breakpoint_inline();
    }

    if (info.conductances.empty()) {
        print_nrn_cur_non_conductance_kernel();
    } else {
        print_nrn_cur_conductance_kernel(node);
    }

    const auto& write_statements = ion_write_statements(BlockType::Equation);
    for (auto& statement: write_statements) {
        auto text = process_shadow_update_statement(statement, BlockType::Equation);
        printer->add_line(text);
    }

    if (info.point_process) {
        const auto& area = get_variable_name(naming::NODE_AREA_VARIABLE);
        printer->fmt_line("double mfactor = 1.e2/{};", area);
        printer->add_line("g = g*mfactor;");
        printer->add_line("rhs = rhs*mfactor;");
    }
}

void CodegenNeuronAccVisitor::print_nrn_cur() {
    if (!nrn_cur_required()) {
        return;
    }

    if (info.conductances.empty()) {
        print_nrn_current(*info.breakpoint_node);
    }

    printer->add_newline(2);
    printer->add_line("/** update current */");
    // Manual function open: capture host t for ACC CURRENT (IClamp window) as
    // firstprivate; device nt->_t is not used (update device(nt._t) is unsafe).
    {
        ParamVector args = {{"", "const _nrn_model_sorted_token&", "", "_sorted_token"},
                            {"", "NrnThread*", "", "nt"},
                            {"", "Memb_list*", "", "_ml_arg"},
                            {"", "int", "", "_type"}};
        printer->fmt_push_block("static void {}({})",
                                compute_method_name(BlockType::Equation),
                                get_parameter_str(args));
    }
    print_kernel_global_device_setup();
    printer->add_line("auto nodecount = _ml_arg->nodecount;");
    printer->add_line("double const _nrn_thread_t = nt->_t;");
    use_host_captured_t_ = true;
    if ((info.net_send_used || info.net_event_used) && !info.artificial_cell) {
        printer->add_line(
            "neuron::gpu::net_send_buffer_ensure_for_events(_ml_arg, nodecount);");
    }
    print_kernel_data_present_annotation_block_begin();
    print_entrypoint_setup_code_from_memb_list();
    use_present_fp_indexing_ = true;

    // Point processes may share a node. Parallel OpenACC needs atomics (Stage 3c)
    // but atomics are non-associative. NRN_DETERMINISTIC_MATRIX=1: serial instance
    // order (matches CPU left-to-right) for testing / raster stability.
    auto print_cur_loop_body = [&](bool det_matrix) {
        force_seq_acc_loop_ = det_matrix;
        print_parallel_iteration_hint(BlockType::Equation, info.breakpoint_node);
        printer->push_block("for (int id = 0; id < nodecount; id++)");
        print_nrn_cur_kernel(*info.breakpoint_node);
        if (info.point_process && !det_matrix) {
            printer->add_line("nrn_pragma_acc(atomic update)");
            printer->add_line("nrn_pragma_omp(atomic update)");
        }
        printer->fmt_line("vec_rhs[node_id] {} rhs;", operator_for_rhs());
        // ELECTRODE sav_rhs is a post-pass (below) — writing sav inside this ACC
        // region with mechanism present caused CUDA illegal address on hh models.
        if (breakpoint_exist()) {
            printer->fmt_line(
                "{} = g;",
                indexed_fp_var(info.vectorize ? naming::CONDUCTANCE_UNUSED_VARIABLE
                                              : naming::CONDUCTANCE_VARIABLE));
        }
        printer->pop_block();
        force_seq_acc_loop_ = false;
    };

    if (info.point_process) {
        printer->push_block("if (neuron::event_order::matrix_enabled())");
        print_cur_loop_body(true);
        printer->chain_block("else");
        print_cur_loop_body(false);
        printer->pop_block();
    } else {
        print_cur_loop_body(false);
    }

    print_after_nrn_cur_gpu_net_send_flush();

    // ELECTRODE_CURRENT → fast_imem sav_rhs after ACC cur (device i is live).
    // Host: serial apply. Device: pull i + sav, scale on host, push sav
    // (in-ACC sav next to mechanism present hit CUDA illegal address on hh).
    if (info.electrode_current && !info.currents.empty()) {
        auto const i_pos = position_of_float_var(info.currents.front());
        printer->push_block("if (auto* vec_sav_rhs = nt->node_sav_rhs_storage())");
        printer->fmt_line("double* _i_col = _lmc.template fpfield_ptr<{}>();", i_pos);
        printer->add_line("auto const* _ni = node_data.nodeindices;");
        printer->add_line("double* _area = nt->node_area_storage();");
        printer->push_block("if (!nt->compute_gpu)");
        printer->push_block("for (int id = 0; id < nodecount; id++)");
        printer->add_line("int node_id = _ni[id];");
        printer->add_line("double mfac = 1.e2 / _area[node_id];");
        printer->fmt_line("vec_sav_rhs[node_id] {} _i_col[id] * mfac;", operator_for_rhs());
        printer->pop_block();
        printer->chain_block("else");
        // Device: wait for cur, pull i, scale on host, RMW sav on device via memcpy.
        // (In-ACC write of sav next to mechanism present hit CUDA illegal address.)
        printer->add_line("nrn_pragma_acc(wait(nt->stream_id))");
        printer->add_line(
            "double* _d_i = static_cast<double*>(acc_deviceptr(_i_col));");
        printer->add_line(
            "double* _d_sav = static_cast<double*>(acc_deviceptr(vec_sav_rhs));");
        printer->push_block("if (_d_i && _d_sav)");
        printer->add_line(
            "std::vector<double> _host_i(static_cast<std::size_t>(nodecount));");
        printer->add_line(
            "acc_memcpy_from_device(_host_i.data(), _d_i, "
            "static_cast<std::size_t>(nodecount) * sizeof(double));");
        printer->add_line(
            "std::vector<double> _host_sav(static_cast<std::size_t>(nt->end));");
        printer->add_line(
            "acc_memcpy_from_device(_host_sav.data(), _d_sav, "
            "static_cast<std::size_t>(nt->end) * sizeof(double));");
        printer->push_block("for (int id = 0; id < nodecount; id++)");
        printer->add_line("int node_id = _ni[id];");
        printer->add_line("double mfac = 1.e2 / _area[node_id];");
        printer->fmt_line("_host_sav[static_cast<std::size_t>(node_id)] {} _host_i["
                          "static_cast<std::size_t>(id)] * mfac;",
                          operator_for_rhs());
        printer->pop_block();
        printer->add_line(
            "acc_memcpy_to_device(_d_sav, _host_sav.data(), "
            "static_cast<std::size_t>(nt->end) * sizeof(double));");
        printer->pop_block();  // _d_i && _d_sav
        printer->pop_block();  // !compute_gpu / else
        printer->pop_block();  // vec_sav_rhs
    }

    print_kernel_data_present_annotation_block_end();
    use_present_fp_indexing_ = false;
    use_host_captured_t_ = false;
    printer->pop_block();
}

void CodegenNeuronAccVisitor::print_entrypoint_setup_code_from_prop() {
    if (info.mod_suffix == "nothing") {
        return;
    }

    printer->add_line("Datum* _ppvar = _nrn_mechanism_access_dparam(prop);");
    printer->add_line("_nrn_mechanism_cache_instance _lmc{prop};");
    printer->add_line("const size_t id = 0;");

    printer->fmt_line("auto inst = make_instance_{}(&_lmc);", info.mod_suffix);
    if (!info.artificial_cell) {
        printer->fmt_line("auto node_data = make_node_data_{}(prop);", info.mod_suffix);
    }

    if (!codegen_thread_variables.empty()) {
        printer->fmt_line("auto _thread_vars = {}({}_global.thread_data);",
                          thread_variables_struct(),
                          info.mod_suffix);
    }

    printer->add_newline();
}

std::string CodegenNeuronAccVisitor::int_variable_name(const IndexVariableInfo& symbol,
                                                       const std::string& name,
                                                       bool use_instance) const {
    if (use_present_fp_indexing_ && use_instance) {
        auto const position = position_of_int_var(name);
        if (info.semantics[position].name == naming::RANDOM_SEMANTIC ||
            info.semantics[position].name == naming::FOR_NETCON_SEMANTIC ||
            info.semantics[position].name == naming::POINTER_SEMANTIC) {
            return CodegenNeuronCppVisitor::int_variable_name(symbol, name, use_instance);
        }
        if (symbol.is_index || symbol.is_integer) {
            return CodegenNeuronCppVisitor::int_variable_name(symbol, name, use_instance);
        }
        // GPU full table: index id + storage_offset (see present_dptr declarations).
        // Host dptr_field_ptr path sets _present_dptr_base_*=0 (already offset).
        return fmt::format("(*_present_dptr_{}[id + _present_dptr_base_{}])", position, position);
    }
    return CodegenNeuronCppVisitor::int_variable_name(symbol, name, use_instance);
}

std::string CodegenNeuronAccVisitor::float_variable_name(const SymbolType& symbol,
                                                         bool use_instance) const {
    if (!use_instance) {
        return CodegenNeuronCppVisitor::float_variable_name(symbol, use_instance);
    }

    const auto& name = symbol->get_name();
    auto const position = position_of_float_var(name);

    // H4c: TABLE rates temps as double& params (_kl_*) or STATE stack locals.
    if (use_present_fp_indexing_ && is_table_statement_float(name)) {
        if (state_kernel_locals_active_) {
            return name;  // stack local in STATE loop
        }
        if (use_kl_ref_in_float_name_) {
            // Procedure / table-update body: double& param.
            return fmt::format("_kl_{}", name);
        }
        // INITIAL / HOC: SoA element (bound into double& at call sites).
        return fmt::format("_present_fp_{}[id]", position);
    }

    if (!use_present_fp_indexing_) {
        if (symbol->is_array()) {
            auto const dimension = symbol->get_length();
            return fmt::format("(_lmc.template data_array_ptr<{}, {}>() + id * {})",
                               position,
                               dimension,
                               dimension);
        }
        return fmt::format("_lmc.template fpfield<{}>(id)", position);
    }
    if (symbol->is_array()) {
        auto const dimension = symbol->get_length();
        return fmt::format("(_present_fp_{} + id * {})", position, dimension);
    }
    return fmt::format("_present_fp_{}[id]", position);
}

std::string CodegenNeuronAccVisitor::get_variable_name(const std::string& name,
                                                       bool use_instance) const {
    // Device net_buf_receive: local `t` is nrb->_nrb_t[index] (event delivery time
    // captured at enqueue). After deliver_net_events(), nt->_t is restored to the
    // step start (tsav), so net_send/net_move must use event t — same as CoreNEURON
    // (`t + time_interval`), not nt->_t + delay (that mis-times self-events).
    if (printing_net_buf_receive_kernel_ && name == "t") {
        return "t";
    }
    // ACC CURRENT/STATE only: host-captured _nrn_thread_t (firstprivate). Do not
    // substitute in INITIAL/net_buf paths that never declare that local.
    if (use_host_captured_t_ && use_present_fp_indexing_ &&
        name == naming::NTHREAD_T_VARIABLE) {
        return "_nrn_thread_t";
    }
    // H4c STATE: stack local `v` instead of v_unused SoA (rates arg + no present).
    if (state_local_v_active_ && (name == "v" || name == naming::VOLTAGE_UNUSED_VARIABLE)) {
        return "v";
    }
    // Force-inlined STATE rates: bare neuron globals (celsius), not *(inst.celsius).
    if (inlining_state_specialized_body_) {
        auto const iter = std::find_if(
            info.neuron_global_variables.begin(),
            info.neuron_global_variables.end(),
            [&name](auto const& entry) { return entry.first->get_name() == name; });
        if (iter != info.neuron_global_variables.end()) {
            return name;
        }
    }
    return CodegenNeuronCppVisitor::get_variable_name(name, use_instance);
}

void CodegenNeuronAccVisitor::print_data_structures(bool print_initializers) {
    print_mechanism_global_var_structure(print_initializers);
    print_mechanism_range_var_structure(false);
    print_node_data_structure(print_initializers);
    print_thread_variables_structure(print_initializers);
    print_make_instance();
    print_make_node_data();
}

void CodegenNeuronAccVisitor::print_make_instance() const {
    printer->add_newline(2);
    printer->fmt_push_block("static {} make_instance_{}(std::size_t data_offset, "
                            "_nrn_mechanism_cache_range* _lmc, "
                            "bool use_device_ptrs = false)",
                            instance_struct(),
                            info.mod_suffix);

    printer->push_block("if(_lmc == nullptr)");
    printer->fmt_line("return {}();", instance_struct());
    printer->pop_block_nl(2);

    printer->fmt_push_block("return {}", instance_struct());

    std::vector<std::string> make_instance_args;

    for (auto const& [var, type]: info.neuron_global_variables) {
        auto const name = var->get_name();
        // Host address only: OpenACC present(hh_global, ...) maps globals; device
        // pointers in inst confuse the compiler's implicit inst.global upload.
        make_instance_args.push_back(fmt::format("&::{0}", name));
    }

    make_instance_args.emplace_back("data_offset");

    const auto codegen_int_variables_size = codegen_int_variables.size();
    for (size_t i = 0; i < codegen_int_variables_size; ++i) {
        const auto& var = codegen_int_variables[i];
        auto name = var.symbol->get_name();
        auto sem = info.semantics[i].name;
        auto const variable = [&var, &sem, i]() -> std::string {
            if (var.is_index || var.is_integer) {
                return "";
            } else if (var.is_vdata) {
                return "";  // RANDOM etc. — not in Instance
            } else if (sem == naming::POINTER_SEMANTIC || sem == naming::RANDOM_SEMANTIC) {
                return "";
            } else {
                return fmt::format("_lmc->template dptr_field_ptr<{}>()", i);
            }
        }();
        if (variable != "") {
            make_instance_args.push_back(variable);
        }
    }

    if (!codegen_global_variables.empty()) {
        make_instance_args.push_back(
            fmt::format("&{0}", global_struct_instance()));
    }

    printer->add_multi_line(fmt::format("{}", fmt::join(make_instance_args, ",\n")));

    printer->pop_block(";");
    printer->pop_block();

    // Host NET_RECEIVE / HOC path: cache_instance (single Prop). Must still fill
    // global/area — incomplete `0` left inst.global null and WATCH+GLOBAL mechs SEGV.
    printer->add_newline(2);
    printer->fmt_push_block("static {} make_instance_{}(_nrn_mechanism_cache_instance* _lmc)",
                            instance_struct(),
                            info.mod_suffix);
    printer->push_block("if(_lmc == nullptr)");
    printer->fmt_line("return {}();", instance_struct());
    printer->pop_block_nl(2);
    printer->fmt_push_block("return {}", instance_struct());
    std::vector<std::string> host_inst_args;
    for (auto const& [var, type]: info.neuron_global_variables) {
        host_inst_args.push_back(fmt::format("&::{0}", var->get_name()));
    }
    host_inst_args.emplace_back("0");  // data_offset
    const auto codegen_int_variables_size_h = codegen_int_variables.size();
    for (size_t i = 0; i < codegen_int_variables_size_h; ++i) {
        const auto& var = codegen_int_variables[i];
        auto sem = info.semantics[i].name;
        if (var.is_index || var.is_integer || var.is_vdata) {
            continue;
        }
        if (sem == naming::POINTER_SEMANTIC || sem == naming::RANDOM_SEMANTIC) {
            continue;
        }
        host_inst_args.push_back(fmt::format("_lmc->template dptr_field_ptr<{}>()", i));
    }
    if (!codegen_global_variables.empty()) {
        host_inst_args.push_back(fmt::format("&{0}", global_struct_instance()));
    }
    printer->add_multi_line(fmt::format("{}", fmt::join(host_inst_args, ",\n")));
    printer->pop_block(";");
    printer->pop_block();
}

void CodegenNeuronAccVisitor::print_make_node_data() const {
    printer->add_newline(2);
    printer->fmt_push_block("static {} make_node_data_{}(NrnThread& nt, Memb_list& _ml_arg, "
                            "bool use_device_ptrs = false)",
                            node_data_struct(),
                            info.mod_suffix);

    // Host pointers: OpenACC present() maps these to device copies inside kernels.
    std::vector<std::string> make_node_data_args = {"_ml_arg.nodeindices",
                                                    "nt.node_voltage_storage()",
                                                    "nt.node_d_storage()",
                                                    "nt.node_rhs_storage()",
                                                    "_ml_arg.nodecount"};

    printer->fmt_push_block("return {}", node_data_struct());
    printer->add_multi_line(fmt::format("{}", fmt::join(make_node_data_args, ",\n")));
    printer->pop_block(";");
    printer->pop_block();

    printer->fmt_push_block("static {} make_node_data_{}(Prop* _prop)",
                            node_data_struct(),
                            info.mod_suffix);

    printer->push_block("if(!_prop)");
    printer->fmt_line("return {}();", node_data_struct());
    printer->pop_block_nl(2);

    printer->add_line("static std::vector<int> node_index{0};");
    printer->add_line("Node* _node = _nrn_mechanism_access_node(_prop);");

    make_node_data_args = {"node_index.data()",
                           "&_nrn_mechanism_access_voltage(_node)",
                           "&_nrn_mechanism_access_d(_node)",
                           "&_nrn_mechanism_access_rhs(_node)",
                           "1"};

    printer->fmt_push_block("return {}", node_data_struct());
    printer->add_multi_line(fmt::format("{}", fmt::join(make_node_data_args, ",\n")));
    printer->pop_block(";");
    printer->pop_block();
    printer->add_newline();
}

void CodegenNeuronAccVisitor::print_entrypoint_setup_code_from_memb_list() {
    if (info.mod_suffix == "nothing") {
        return;
    }

    printer->add_line(
        "_nrn_mechanism_cache_range _lmc{_sorted_token, *nt, *_ml_arg, _ml_arg->type()};");
    printer->fmt_line("auto inst = make_instance_{}(_ml_arg->get_storage_offset(), &_lmc, "
                      "nt->compute_gpu);",
                      info.mod_suffix);
    if (!info.artificial_cell) {
        printer->fmt_line("auto node_data = make_node_data_{}(*nt, *_ml_arg, nt->compute_gpu);",
                          info.mod_suffix);
    }
    printer->add_line("auto* _thread = _ml_arg->_thread;");
    if (!codegen_thread_variables.empty()) {
        printer->fmt_line("auto _thread_vars = {}(_thread[{}].get<double*>());",
                          thread_variables_struct(),
                          info.thread_var_thread_id);
    }
}

void CodegenNeuronAccVisitor::visit_watch_statement(const ast::WatchStatement& node) {
    // Device net_buf_receive cannot arm host WatchCondition. WATCH mechs use
    // host NET_RECEIVE (see print_net_receive); skip WATCH in the device kernel.
    if (printing_net_buf_receive_kernel_) {
        return;
    }
    CodegenNeuronCppVisitor::visit_watch_statement(node);
}

}  // namespace codegen
}  // namespace nmodl