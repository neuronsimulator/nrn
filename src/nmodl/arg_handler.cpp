/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "arg_handler.hpp"
#include "tclap/CmdLine.h"
#include "utils/string_utils.hpp"
#include "version/version.h"


namespace nmodl {

ArgumentHandler::ArgumentHandler(const int& argc, const char** argv) {
    // version string
    auto version = nmodl::version().to_string();

    try {
        using string_vector_type = std::vector<std::string>;
        using value_constraint_type = TCLAP::ValuesConstraint<std::string>;
        using value_arg_type = TCLAP::ValueArg<std::string>;
        using switch_arg_type = TCLAP::SwitchArg;
        using unlabel_arg_type = TCLAP::UnlabeledMultiArg<std::string>;

        TCLAP::CmdLine cmd("NMODL :: Code Generator Toolkit for NMODL", ' ', version);

        // clang-format off
        string_vector_type host_val = {"SERIAL", "OPENMP", "OPENACC"};
        value_constraint_type host_constr(host_val);
        value_arg_type host_arg(
                "",
                "host",
                "Host / CPU backend [" + host_val[0] +"]",
                false,
                host_val[0],
                &host_constr,
                cmd);

        string_vector_type accel_val = {"CUDA"};
        value_constraint_type accel_constr(accel_val);
        value_arg_type accel_arg(
                "",
                "accelerator",
                "Accelerator / Device backend [" + accel_val[0] + "]",
                false,
                "",
                &accel_constr,
                cmd);

        string_vector_type dtype_val = {"DOUBLE", "FLOAT"};
        value_constraint_type dtype_constr(dtype_val);
        value_arg_type dtype_arg(
                "",
                "datatype",
                "Floating point data type [" + dtype_val[0] + "]",
                false,
                dtype_val[0],
                &dtype_constr,
                cmd);

        string_vector_type layout_val = {"SOA", "AOS"};
        value_constraint_type layout_constr(layout_val);
        value_arg_type layout_arg(
                "",
                "layout",
                "Memory layout for channel/synapse [" + layout_val[0] +"]",
                false,
                layout_val[0],
                &layout_constr,
                cmd);

        value_arg_type output_dir_arg(
                "",
                "output-dir",
                "Output directory for code generator [.]",
                false,
                ".",
                "string",
                cmd);

        value_arg_type scratch_dir_arg(
                "",
                "scratch-dir",
                "Output directory for intermediate results [tmp]",
                false,
                "tmp",
                "string",
                cmd);

        switch_arg_type verbose_arg(
                "",
                "verbose",
                "Enable verbose output",
                cmd,
                false);

        switch_arg_type sympy_arg(
                "",
                "enable-sympy",
                "Enable SymPy analytic integration",
                cmd,
                false);

        switch_arg_type pade_approx_arg(
                "",
                "enable-pade-approx",
                "Enable Pade Approx in SymPy analytic integration",
                cmd,
                false);

        switch_arg_type inline_arg(
                "",
                "nmodl-inline",
                "Enable NMODL inlining",
                cmd,
                false);

        switch_arg_type localize_arg(
                "",
                "localize",
                "Localize RANGE, GLOBAL, ASSIGNED variables",
                cmd,
                false);

        switch_arg_type localize_verbatim_arg(
                "",
                "localize-verbatim",
                "Localize even with VERBATIM blocks (unsafe optimizations)",
                cmd,
                false);

        switch_arg_type local_rename_arg(
                "",
                "local-rename",
                "Rename LOCAL variables if necessary",
                cmd,
                false);

        switch_arg_type perf_stats_arg(
                "",
                "dump-perf-stats",
                "Run performance visitor and dump stats into JSON file",
                cmd,
                false);

        switch_arg_type nmodl_state_arg(
                "",
                "dump-nmodl-state",
                "Dump intermediate AST states into NMODL",
                cmd,
                false);

        switch_arg_type ast_to_json_arg(
                "",
                "dump-ast-as-json",
                "Dump intermediate AST states into JSON state",
                cmd,
                false);

        switch_arg_type no_verbatim_rename_arg(
                "",
                "no-verbatim-rename",
                "Disable renaming variables in VERBATIM block",
                cmd,
                false);

        switch_arg_type show_symtab_arg(
                "",
                "show-symtab",
                "Show symbol table to stdout",
                cmd,
                false);

        unlabel_arg_type nmodl_arg(
                "files",
                "NMODL input models path",
                true,
                "string",
                cmd);
        // clang-format on

        cmd.parse(argc, argv);

        nmodl_files = nmodl_arg.getValue();
        host_backend = host_arg.getValue();
        accel_backend = accel_arg.getValue();
        dtype = stringutils::tolower(dtype_arg.getValue());
        mlayout = layout_arg.getValue();
        verbose = verbose_arg.getValue();
        sympy = sympy_arg.getValue();
        pade_approx = pade_approx_arg.getValue();
        inlining = inline_arg.getValue();
        localize_with_verbatim = localize_verbatim_arg.getValue();
        local_rename = local_rename_arg.getValue();
        verbatim_rename = !no_verbatim_rename_arg.getValue();
        localize = localize_arg.getValue();
        perf_stats = perf_stats_arg.getValue();
        show_symtab = show_symtab_arg.getValue();
        output_dir = output_dir_arg.getValue();
        scratch_dir = scratch_dir_arg.getValue();
        ast_to_nmodl = nmodl_state_arg.getValue();
        ast_to_json = ast_to_json_arg.getValue();
    } catch (TCLAP::ArgException& e) {
        std::cout << "Argument Error: " << e.error() << " for arg " << e.argId() << std::endl;
    }
}

}  // namespace nmodl
