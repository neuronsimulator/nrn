/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/
#pragma once
#include <memory>
#include <ostream>
#include <string>

/**
 * \class corenrn_parameters
 * \brief Parses and contains Command Line parameters for Core Neuron
 *
 * This structure contains all the parameters that CoreNeuron fetches
 * from the Command Line. It uses the CLI11 libraries to parse these parameters
 * and saves them in an internal public structure. Each parameter can be
 * accessed or written freely. By default the constructor instantiates a
 * CLI11 object and initializes it for CoreNeuron use.
 * This object is freely accessible from any point of the program.
 * An ostream method is also provided to print out all the parameters that
 * CLI11 parse.
 * Please keep in mind that, due to the nature of the subcommands in CLI11,
 * the command line parameters for subcategories NEED to be come before the relative
 * parameter. e.g. --mpi --gpu gpu --nwarp
 * Also single dash long options are not supported anymore (-mpi -> --mpi).
 */

namespace CLI {
struct App;
}

namespace coreneuron {

struct corenrn_parameters_data {
    enum verbose_level : std::uint32_t {
        NONE = 0,
        ERROR = 1,
        INFO = 2,
        DEBUG_INFO = 3,
        DEFAULT = INFO
    };

    static constexpr int report_buff_size_default = 4;

    unsigned spikebuf = 100'000;           /// Internal buffer used on every rank for spikes
    int prcellgid = -1;                    /// Gid of cell for prcellstate
    unsigned ms_phases = 2;                /// Number of multisend phases, 1 or 2
    unsigned ms_subint = 2;                /// Number of multisend interval. 1 or 2
    unsigned spkcompress = 0;              /// Spike Compression
    unsigned cell_interleave_permute = 0;  /// Cell interleaving permutation
    unsigned nwarp = 65536;  /// Number of warps to balance for cell_interleave_permute == 2
    unsigned num_gpus = 0;   /// Number of gpus to use per node
    unsigned report_buff_size = report_buff_size_default;  /// Size in MB of the report buffer.
    int seed = -1;  /// Initialization seed for random number generator (int)

    bool mpi_enable = false;         /// Enable MPI flag.
    bool skip_mpi_finalize = false;  /// Skip MPI finalization
    bool multisend = false;          /// Use Multisend spike exchange instead of Allgather.
    bool threading = false;          /// Enable pthread/openmp
    bool gpu = false;                /// Enable GPU computation.
    bool cuda_interface = false;     /// Enable CUDA interface (default is the OpenACC interface).
                                  /// Branch of the code is executed through CUDA kernels instead of
                                  /// OpenACC regions.
    bool binqueue = false;  /// Use bin queue.

    bool show_version = false;  /// Print version and exit.

    bool model_stats = false;  /// Print mechanism counts and model size after initialization

    verbose_level verbose{verbose_level::DEFAULT};  /// Verbosity-level

    double tstop = 100;        /// Stop time of simulation in msec
    double dt = -1000.0;       /// Timestep to use in msec
    double dt_io = 0.1;        /// I/O timestep to use in msec
    double dt_report;          /// I/O timestep to use in msec for reports
    double celsius = -1000.0;  /// Temperature in degC.
    double voltage = -65.0;    /// Initial voltage used for nrn_finitialize(1, v_init).
    double forwardskip = 0.;   /// Forward skip to TIME.
    double mindelay = 10.;     /// Maximum integration interval (likely reduced by minimum NetCon
                               /// delay).

    std::string patternstim;             /// Apply patternstim using the specified spike file.
    std::string datpath = ".";           /// Directory path where .dat files
    std::string outpath = ".";           /// Directory where spikes will be written
    std::string filesdat = "files.dat";  /// Name of file containing list of gids dat files read in
    std::string restorepath;             /// Restore simulation from provided checkpoint directory.
    std::string reportfilepath;          /// Reports configuration file.
    std::string checkpointpath;  /// Enable checkpoint and specify directory to store related files.
    std::string writeParametersFilepath;  /// Write parameters to this file
    std::string mpi_lib;                  /// Name of CoreNEURON MPI library to load dynamically.
};

struct corenrn_parameters: corenrn_parameters_data {
    corenrn_parameters();   /// Constructor that initializes the CLI11 app.
    ~corenrn_parameters();  /// Destructor defined in .cpp where CLI11 types are complete.

    void parse(int argc, char* argv[]);  /// Runs the CLI11_PARSE macro.

    /** @brief Reset all parameters to their default values.
     *
     *  Unfortunately it is awkward to support `x = corenrn_parameters{}`
     *  because `app` holds pointers to members of `corenrn_parameters`.
     */
    void reset();

    inline bool is_quiet() {
        return verbose == verbose_level::NONE;
    }

    /** @brief Return a string summarising the current parameter values.
     *
     * This forwards to the CLI11 method of the same name. Returns a string that
     * could be read in as a config of the current values of the App.
     *
     * @param default_also Include any defaulted arguments.
     * @param write_description Include option descriptions and the App description.
     */
    std::string config_to_str(bool default_also = false, bool write_description = false) const;

  private:
    // CLI app that performs CLI parsing. std::unique_ptr avoids having to
    // include CLI11 headers from CoreNEURON headers, and therefore avoids
    // CoreNEURON having to install CLI11 when using it from a submodule.
    std::unique_ptr<CLI::App> m_app;
};

std::ostream& operator<<(std::ostream& os,
                         const corenrn_parameters& corenrn_param);  /// Printing method.

extern corenrn_parameters corenrn_param;  /// Declaring global corenrn_parameters object for this
                                          /// instance of CoreNeuron.
extern int nrn_nobanner_;                 /// Global no banner setting

}  // namespace coreneuron
