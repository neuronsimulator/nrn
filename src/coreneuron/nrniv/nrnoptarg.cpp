/*
Copyright (c) 2016, Blue Brain Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "coreneuron/nrnmpi/nrnmpi.h"
#include "coreneuron/nrniv/nrnoptarg.h"
#include "coreneuron/utils/ezoption/ezOptionParser.hpp"
namespace coreneuron {
struct param_int {
    const char* names; /* space separated (includes - or --) */
    int dflt, low, high;
    const char* usage;
};

struct param_dbl {
    const char* names; /* space separated (includes - or --) */
    double dflt, low, high;
    const char* usage;
};

struct param_flag {
    const char* names; /* space separated (includes - or --) */
    const char* usage;
};

struct param_str {
    const char* names; /* space separated (includes - or --) */
    const char* dflt;
    const char* usage;
};

static param_int param_int_args[] = {
    {"--spikebuf -b", 100000, 1, 2000000000, "Spike buffer size. (100000)"},
    {"--spkcompress", 0, 0, 100000,
     "Spike compression. Up to ARG are exchanged during MPI_Allgather. (0)"},
    {"--prcellgid -g", -1, -1, 2000000000, "Output prcellstate information for the gid NUMBER."},
    {"--cell-permute -R", 0, 0, 3,
     "Cell permutation, 0 No; 1 optimise node adjacency; 2 optimize parent adjacency. (1)"},
    {"--nwarp -W", 0, 0, 1000000, "Number of warps to balance. (0)"},
    {"--ms-subintervals", 2, 1, 2, "Number of multisend subintervals, 1 or 2. (2)"},
    {"--ms-phases", 2, 1, 2, "Number of multisend phases, 1 or 2. (2)"},
    {"--multiple -z", 1, 1, 10000000,
     "Model duplication factor. Model size is normal size * (int)."},
    {"--extracon -x", 0, 0, 10000000,
     "Number of extra random connections in each thread to other duplicate models (int)."},
    {"--seed -s", -1, 0, 100000000, "Initialization seed for random number generator (int)."},
    {"--report-buffer-size", 4, 1, 128, "Size in MB of the report buffer (int)."},
    {nullptr, 0, 0, 0, nullptr}};

static param_dbl param_dbl_args[] = {
    {"--tstop -e", 100.0, 0.0, 1e9, "Stop time (ms). (100)"},
    {"--dt -dt", -1000., -1000., 1e9,
     "Fixed time step. The default value is set by defaults.dat or is 0.025."},
    {"--dt_io -i", 0.1, 1e-9, 1e9, "Dt of I/O. (0.1)"},
    {"--voltage -v", -65.0, -1e9, 1e9,
     "Initial voltage used for nrn_finitialize(1, v_init). If 1000, then nrn_finitialize(0,...). (-65.)"},
    {"--celsius -l", -1000., -1000., 1000.,
     "Temperature in degC. The default value is set in defaults.dat or else is 34.0."},
    {"--forwardskip -k", 0., 0., 1e9, "Forwardskip to TIME"},
    {"--mindelay", 10., 0., 1e9,
     "Maximum integration interval (likely reduced by minimum NetCon delay). (10)"},
    {nullptr, 0., 0., 0., nullptr}};

static param_flag param_flag_args[] = {
    {"--help -h",
     "Print a usage message briefly summarizing these command-line options, then exit."},
    {"--threading -c", "Parallel threads. The default is serial threads."},
    {"--gpu -gpu", "Enable use of GPUs. The default implies cpu only run."},
    {"-mpi", "Enable MPI. In order to initialize MPI environment this argument must be specified."},
    {"--show", "Print args."},
    {"--multisend", "Use Multisend spike exchange instead of Allgather."},
    {"--binqueue", "Use bin queue."},
    {"--skip-mpi-finalize", "Do not call mpi finalize."},
    {nullptr, nullptr}};

static param_str param_str_args[] = {
    {"--pattern -p", "", "Apply patternstim using the specified spike file."},
    {"--datpath -d", ".", "Path containing CoreNeuron data files. (.)"},
    {"--checkpoint", "", "Enable checkpoint and specify directory to store related files."},
    {"--restore", "", "Restore simulation from provided checkpoint directory."},
    {"--filesdat -f", "files.dat", "Name for the distribution file. (files.dat)"},
    {"--outpath -o", ".", "Path to place output data files. (.)"},
    {"--write-config", "", "Write configuration file filename."},
    {"--read-config", "", "Read configuration file filename."},
    {"--report-conf", "", "reports configuration file"},
    {nullptr, nullptr, nullptr}};

static void graceful_exit(int);

static ez::ezOptionParser* opt;

int nrnopt_parse(int argc, const char* argv[]) {
    opt = new ez::ezOptionParser;

    for (int i = 0; param_int_args[i].names; ++i) {
        struct param_int& p = param_int_args[i];
        nrnopt_add_int(p.names, p.usage, p.dflt, p.low, p.high);
    }

    for (int i = 0; param_str_args[i].names; ++i) {
        struct param_str& p = param_str_args[i];
        nrnopt_add_str(p.names, p.usage, p.dflt);
    }

    for (int i = 0; param_dbl_args[i].names; ++i) {
        struct param_dbl& p = param_dbl_args[i];
        nrnopt_add_dbl(p.names, p.usage, p.dflt, p.low, p.high);
    }

    for (int i = 0; param_flag_args[i].names; ++i) {
        struct param_flag& p = param_flag_args[i];
        nrnopt_add_flag(p.names, p.usage);
    }

    // note earliest takes precedence.

    // first the command line arguments
    opt->parse(argc, argv);

    // then any specific configuration files mentioned in the command line
    if (opt->isSet("--read-config")) {
        // Import one or more files that use # as comment char.
        std::vector<std::vector<std::string> > files;
        opt->get("--read-config")->getMultiStrings(files);

        for (size_t j = 0; j < files.size(); ++j) {
            if (!opt->importFile(files[j][0].c_str(), '#')) {
                if (nrnmpi_myid == 0)
                    std::cerr << "ERROR: Failed to read configuration file " << files[j][0]
                              << std::endl;
                graceful_exit(1);
            }
        }
    }

    std::string usage;
    std::vector<std::string> badOptions;

    if (!opt->gotExpected(badOptions)) {
        for (size_t i = 0; i < badOptions.size(); ++i) {
            if (nrnmpi_myid)
                std::cerr << "ERROR: Got unexpected number of arguments for option "
                          << badOptions[i] << ".\n\n";
        }

        opt->getUsage(usage);
        if (nrnmpi_myid == 0)
            std::cout << usage;
        graceful_exit(1);
    }

    if (opt->isSet("-h")) {
        opt->getUsage(usage);
        if (nrnmpi_myid == 0)
            std::cout << usage;
        graceful_exit(0);
    }

    if (opt->firstArgs.size() > 1 || opt->lastArgs.size() > 0 || opt->unknownArgs.size() > 0) {
        std::cerr << "ERROR: Unknown arguments"
                  << "\n";
        for (size_t i = 1; i < opt->firstArgs.size(); ++i) {
            if (nrnmpi_myid == 0)
                std::cerr << "   " << opt->firstArgs[i]->c_str() << "\n";
        }
        for (size_t i = 0; i < opt->unknownArgs.size(); ++i) {
            if (nrnmpi_myid == 0)
                std::cerr << "   " << opt->unknownArgs[i]->c_str() << "\n";
        }
        for (size_t i = 0; i < opt->lastArgs.size(); ++i) {
            if (nrnmpi_myid == 0)
                std::cerr << "   " << opt->lastArgs[i]->c_str() << "\n";
        }
        graceful_exit(1);
    }

    std::vector<std::string> badArgs;
    if (!opt->gotValid(badOptions, badArgs)) {
        for (size_t i = 0; i < badOptions.size(); ++i) {
            if (nrnmpi_myid == 0)
                std::cerr << "ERROR: Got invalid argument \"" << badArgs[i] << "\" for option "
                          << badOptions[i] << ".\n";
        }
        graceful_exit(1);
    }

    if (opt->isSet("--write-config")) {
        std::string file;
        opt->get("--write-config")->getString(file);
        // Exports all options if second param is true; unset options will just use their default
        // values.
        if (!opt->exportFile(file.c_str(), true)) {
            if (nrnmpi_myid == 0)
                std::cerr << "ERROR: Failed to write to configuration file " << file.c_str()
                          << std::endl;
            graceful_exit(1);
        }
    }

    if (opt->isSet("--show")) {
        nrnopt_show();
    }

    return 0;
}

static void nrnopt_add(const char* names,
                       const char* usage,
                       const char* dflt,
                       ez::ezOptionValidator* validator) {
    std::vector<std::string> vnames;
    ez::SplitDelim(std::string(names), ' ', vnames);
    int expect = dflt ? 1 : 0;
    dflt = dflt ? dflt : "";
    if (vnames.size() == 1) {
        opt->add(dflt,
                 0,                  // Required?
                 expect,             // Number of args expected
                 0,                  // Delimiter if expecting multiple args
                 usage,              // Help description
                 vnames[0].c_str(),  // flag name
                 validator);
    } else if (vnames.size() == 2) {
        opt->add(dflt, 0, expect, 0, usage, vnames[0].c_str(), vnames[1].c_str(), validator);
    }
}

void nrnopt_add_flag(const char* names, const char* usage) {
    nrnopt_add(names, usage, nullptr, nullptr);
}

void nrnopt_add_str(const char* names, const char* usage, const char* dflt) {
    nrnopt_add(names, usage, dflt, nullptr);
}

void nrnopt_add_int(const char* names, const char* usage, int dflt, int low, int high) {
    char s1[50], s2[100];
    sprintf(s1, "%d", dflt);
    sprintf(s2, "%d,%d", low, high);
    ez::ezOptionValidator* v = new ez::ezOptionValidator("s4", "gele", s2);
    nrnopt_add(names, usage, s1, v);
}

void nrnopt_add_dbl(const char* names, const char* usage, double dflt, double low, double high) {
    char s1[50], s2[100];
    sprintf(s1, "%.16g", dflt);
    sprintf(s2, "%.17g,%.17g", low, high);
    ez::ezOptionValidator* v = new ez::ezOptionValidator("f", "gele", s2);
    nrnopt_add(names, usage, s1, v);
}

bool nrnopt_get_flag(const char* name) {
    bool flag = opt->isSet(name) ? true : false;
    return flag;
}

std::string nrnopt_get_str(const char* name) {
    std::string val;
    opt->get(name)->getString(val);
    return val;
}

int nrnopt_get_int(const char* name) {
    int val;
    opt->get(name)->getInt(val);
    return val;
}

double nrnopt_get_dbl(const char* name) {
    double val;
    opt->get(name)->getDouble(val);
    return val;
}

void nrnopt_modify_dbl(const char* name, double val) {
    ez::OptionGroup* og = opt->get(name);
    og->isSet = true;
    std::vector<std::vector<std::string*>*>& args = og->args;
    std::vector<std::string*>* v = new std::vector<std::string*>;
    char c[50];
    sprintf(c, "%.16g", val);
    std::string* s = new std::string(c);
    v->push_back(s);
    args.insert(args.begin(), v);
}

bool nrnopt_is_default_value(const char* name) {
    return !opt->isSet(name);
}

// more compact display of parameters.
// ie only first name and first value (or default or isSet).
void nrnopt_show() {
    if (nrnmpi_myid) {
        return;
    }
    std::string out;

    int n = opt->groups.size();
    for (int i = 0; i < n; ++i) {
        if (i % 3 == 0) {
            std::cout << out << std::endl;
            out.clear();
        } else {
            int n = (i % 3) * 25;
            for (int i = out.size(); i < n; ++i) {
                out += " ";
            }
        }
        ez::OptionGroup* g = opt->groups[i];
        out += g->flags[0]->c_str();
        out += " = ";
        if (g->args.size()) {
            std::string s;
            g->getString(s);
            out += s.c_str();
        } else if (g->defaults.size() > 0) {
            out += g->defaults.c_str();
        } else {
            out += (g->isSet ? "set" : "not set");
        }
    }
    if (out.size() > 0) {
        std::cout << out << std::endl;
    }
    std::cout << std::endl;
}

void nrnopt_delete() {
    delete opt;
    opt = nullptr;
}

static void graceful_exit(int err) {
#if !defined(nrnoptargtest) && NRNMPI
    // actually, only avoid mpi message when all ranks exit(0)
    nrnmpi_finalize();
#endif
    exit(nrnmpi_myid == 0 ? err : 0);
}
}  // namespace coreneuron
