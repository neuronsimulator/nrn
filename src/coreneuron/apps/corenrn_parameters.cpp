/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#include "coreneuron/apps/corenrn_parameters.hpp"


namespace coreneuron {

extern std::string cnrn_version();

corenrn_parameters::corenrn_parameters() {
    app.set_config("--read-config", "", "Read parameters from ini file", false)
        ->check(CLI::ExistingFile);
    app.add_option("--write-config",
                   this->writeParametersFilepath,
                   "Write parameters to this file",
                   false);

    app.add_flag(
        "--mpi",
        this->mpi_enable,
        "Enable MPI. In order to initialize MPI environment this argument must be specified.");
    app.add_flag("--gpu", this->gpu, "Activate GPU computation.");
    app.add_option("--dt",
                   this->dt,
                   "Fixed time step. The default value is set by defaults.dat or is 0.025.",
                   true)
        ->check(CLI::Range(-1'000., 1e9));
    app.add_option("-e, --tstop", this->tstop, "Stop Time in ms.")->check(CLI::Range(0., 1e9));
    app.add_flag("--show");
    app.add_set(
        "--verbose",
        this->verbose,
        {verbose_level::NONE, verbose_level::ERROR, verbose_level::INFO, verbose_level::DEBUG},
        "Verbose level: 0 = NONE, 1 = ERROR, 2 = INFO, 3 = DEBUG. Default is INFO");
    app.add_flag("--model-stats",
                 this->model_stats,
                 "Print number of instances of each mechanism and detailed memory stats.");

    auto sub_gpu = app.add_option_group("GPU", "Commands relative to GPU.");
    sub_gpu->add_option("-W, --nwarp", this->nwarp, "Number of warps to balance.", true)
        ->check(CLI::Range(0, 1'000'000));
    sub_gpu
        ->add_option("-R, --cell-permute",
                     this->cell_interleave_permute,
                     "Cell permutation: 0 No permutation; 1 optimise node adjacency; 2 optimize "
                     "parent adjacency.",
                     true)
        ->check(CLI::Range(0, 3));
    sub_gpu->add_flag("--cuda-interface",
                      this->cuda_interface,
                      "Activate CUDA branch of the code.");

    auto sub_input = app.add_option_group("input", "Input dataset options.");
    sub_input->add_option("-d, --datpath", this->datpath, "Path containing CoreNeuron data files.")
        ->check(CLI::ExistingDirectory);
    sub_input->add_option("-f, --filesdat", this->filesdat, "Name for the distribution file.", true)
        ->check(CLI::ExistingFile);
    sub_input
        ->add_option("-p, --pattern",
                     this->patternstim,
                     "Apply patternstim using the specified spike file.")
        ->check(CLI::ExistingFile);
    sub_input
        ->add_option("-s, --seed", this->seed, "Initialization seed for random number generator.")
        ->check(CLI::Range(0, 100'000'000));
    sub_input
        ->add_option("-v, --voltage",
                     this->voltage,
                     "Initial voltage used for nrn_finitialize(1, v_init). If 1000, then "
                     "nrn_finitialize(0,...).")
        ->check(CLI::Range(-1e9, 1e9));
    sub_input->add_option("--report-conf", this->reportfilepath, "Reports configuration file.")
        ->check(CLI::ExistingFile);
    sub_input
        ->add_option("--restore",
                     this->restorepath,
                     "Restore simulation from provided checkpoint directory.")
        ->check(CLI::ExistingDirectory);

    auto sub_parallel = app.add_option_group("parallel", "Parallel processing options.");
    sub_parallel->add_flag("-c, --threading",
                           this->threading,
                           "Parallel threads. The default is serial threads.");
    sub_parallel->add_flag("--skip-mpi-finalize",
                           this->skip_mpi_finalize,
                           "Do not call mpi finalize.");

    auto sub_spike = app.add_option_group("spike", "Spike exchange options.");
    sub_spike
        ->add_option("--ms-phases", this->ms_phases, "Number of multisend phases, 1 or 2.", true)
        ->check(CLI::Range(1, 2));
    sub_spike
        ->add_option("--ms-subintervals",
                     this->ms_subint,
                     "Number of multisend subintervals, 1 or 2.",
                     true)
        ->check(CLI::Range(1, 2));
    sub_spike->add_flag("--multisend",
                        this->multisend,
                        "Use Multisend spike exchange instead of Allgather.");
    sub_spike
        ->add_option("--spkcompress",
                     this->spkcompress,
                     "Spike compression. Up to ARG are exchanged during MPI_Allgather.",
                     true)
        ->check(CLI::Range(0, 100'000));
    sub_spike->add_flag("--binqueue", this->binqueue, "Use bin queue.");

    auto sub_config = app.add_option_group("config", "Config options.");
    sub_config->add_option("-b, --spikebuf", this->spikebuf, "Spike buffer size.", true)
        ->check(CLI::Range(0, 2'000'000'000));
    sub_config
        ->add_option("-g, --prcellgid",
                     this->prcellgid,
                     "Output prcellstate information for the gid NUMBER.")
        ->check(CLI::Range(-1, 2'000'000'000));
    sub_config->add_option("-k, --forwardskip", this->forwardskip, "Forwardskip to TIME")
        ->check(CLI::Range(0., 1e9));
    sub_config
        ->add_option(
            "-l, --celsius",
            this->celsius,
            "Temperature in degC. The default value is set in defaults.dat or else is 34.0.",
            true)
        ->check(CLI::Range(-1000., 1000.));
    sub_config
        ->add_option("--mindelay",
                     this->mindelay,
                     "Maximum integration interval (likely reduced by minimum NetCon delay).",
                     true)
        ->check(CLI::Range(0., 1e9));
    sub_config
        ->add_option("--report-buffer-size",
                     this->report_buff_size,
                     "Size in MB of the report buffer.")
        ->check(CLI::Range(1, 128));

    auto sub_output = app.add_option_group("output", "Output configuration.");
    sub_output->add_option("-i, --dt_io", this->dt_io, "Dt of I/O.", true)
        ->check(CLI::Range(-1000., 1e9));
    sub_output->add_option("-o, --outpath",
                           this->outpath,
                           "Path to place output data files.",
                           true);
    sub_output->add_option("--checkpoint",
                           this->checkpointpath,
                           "Enable checkpoint and specify directory to store related files.");

    app.add_flag("-v, --version", this->show_version, "Show version information and quit.");

    CLI::retire_option(app, "--show");
};

void corenrn_parameters::parse(int argc, char** argv) {
    try {
        app.parse(argc, argv);
        if (verbose == verbose_level::NONE) {
            nrn_nobanner_ = 1;
        }
    } catch (const CLI::ExtrasError& e) {
        // in case of parsing errors, show message with exception
        std::cerr << "CLI parsing error, see nrniv-core --help for more information. \n"
                  << std::endl;
        app.exit(e);
        throw e;

    } catch (const CLI::ParseError& e) {
        // use --help is also ParseError; in this case exit by showing all options
        app.exit(e);
        exit(0);
    }

    // is user has asked for version info, print it and exit
    if (show_version) {
        std::cout << "CoreNEURON Version : " << cnrn_version() << std::endl;
        exit(0);
    }
};

std::ostream& operator<<(std::ostream& os, const corenrn_parameters& corenrn_param) {
    os << "GENERAL PARAMETERS" << std::endl
       << "--mpi=" << (corenrn_param.mpi_enable ? "true" : "false") << std::endl
       << "--gpu=" << (corenrn_param.gpu ? "true" : "false") << std::endl
       << "--dt=" << corenrn_param.dt << std::endl
       << "--tstop=" << corenrn_param.tstop << std::endl
       << std::endl
       << "GPU" << std::endl
       << "--nwarp=" << corenrn_param.nwarp << std::endl
       << "--cell-permute=" << corenrn_param.cell_interleave_permute << std::endl
       << "--cuda-interface=" << (corenrn_param.cuda_interface ? "true" : "false") << std::endl
       << std::endl
       << "INPUT PARAMETERS" << std::endl
       << "--voltage=" << corenrn_param.voltage << std::endl
       << "--seed=" << corenrn_param.seed << std::endl
       << "--datpath=" << corenrn_param.datpath << std::endl
       << "--filesdat=" << corenrn_param.filesdat << std::endl
       << "--pattern=" << corenrn_param.patternstim << std::endl
       << "--report-conf=" << corenrn_param.reportfilepath << std::endl
       << std::left << std::setw(15) << "--restore=" << corenrn_param.restorepath << std::endl
       << std::endl
       << "PARALLEL COMPUTATION PARAMETERS" << std::endl
       << "--threading=" << (corenrn_param.threading ? "true" : "false") << std::endl
       << "--skip_mpi_finalize=" << (corenrn_param.skip_mpi_finalize ? "true" : "false")
       << std::endl
       << std::endl
       << "SPIKE EXCHANGE" << std::endl
       << "--ms_phases=" << corenrn_param.ms_phases << std::endl
       << "--ms_subintervals=" << corenrn_param.ms_subint << std::endl
       << "--multisend=" << (corenrn_param.multisend ? "true" : "false") << std::endl
       << "--spk_compress=" << corenrn_param.spkcompress << std::endl
       << "--binqueue=" << (corenrn_param.binqueue ? "true" : "false") << std::endl
       << std::endl
       << "CONFIGURATION" << std::endl
       << "--spikebuf=" << corenrn_param.spikebuf << std::endl
       << "--prcellgid=" << corenrn_param.prcellgid << std::endl
       << "--forwardskip=" << corenrn_param.forwardskip << std::endl
       << "--celsius=" << corenrn_param.celsius << std::endl
       << "--mindelay=" << corenrn_param.mindelay << std::endl
       << "--report-buffer-size=" << corenrn_param.report_buff_size << std::endl
       << std::endl
       << "OUTPUT PARAMETERS" << std::endl
       << "--dt_io=" << corenrn_param.dt_io << std::endl
       << "--outpath=" << corenrn_param.outpath << std::endl
       << "--checkpoint=" << corenrn_param.checkpointpath << std::endl;

    return os;
}

corenrn_parameters corenrn_param;
int nrn_nobanner_{0};

}  // namespace coreneuron
