#include "neuron/gpu/device_assign.hpp"

#include "neuron/gpu/config.hpp"
#include "neuron/gpu/offload.hpp"

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>

#if NRNMPI
#include <mpi.h>
// Note: do NOT use MPI_Comm_split_type here — assign_device can run from
// gpu.enable on each rank without a collective barrier (Python startup order
// differs). Collectives hang. Use launcher env (OpenMPI/MPICH/PMI) like many
// GPU apps; fall back to world rank only when a single process is local.
#endif

namespace neuron::gpu {
namespace {

int env_int(char const* name, int default_value) {
    char const* v = std::getenv(name);
    if (!v || !v[0]) {
        return default_value;
    }
    return std::atoi(v);
}

int mpi_simulation_rank() {
#if NRNMPI
    int flag = 0;
    if (MPI_Initialized(&flag) == MPI_SUCCESS && flag) {
        int rank = 0;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        return rank;
    }
#endif
    // Launchers export world rank even if MPI_Init order is odd.
    int r = env_int("OMPI_COMM_WORLD_RANK", -1);
    if (r >= 0) {
        return r;
    }
    r = env_int("PMI_RANK", -1);
    if (r >= 0) {
        return r;
    }
    return 0;
}

/** Node-local rank for GPU round-robin (non-collective). */
int mpi_local_rank() {
    // Open MPI
    int r = env_int("OMPI_COMM_WORLD_LOCAL_RANK", -1);
    if (r >= 0) {
        return r;
    }
    // MPICH / Intel MPI
    r = env_int("MPI_LOCALRANKID", -1);
    if (r >= 0) {
        return r;
    }
    r = env_int("MV2_COMM_WORLD_LOCAL_RANK", -1);
    if (r >= 0) {
        return r;
    }
    // SLURM
    r = env_int("SLURM_LOCALID", -1);
    if (r >= 0) {
        return r;
    }
    return 0;
}

/** Ranks on this shared-memory node (non-collective). */
int mpi_local_size() {
    int s = env_int("OMPI_COMM_WORLD_LOCAL_SIZE", -1);
    if (s > 0) {
        return s;
    }
    s = env_int("MPI_LOCALNRANKS", -1);
    if (s > 0) {
        return s;
    }
    s = env_int("MV2_COMM_WORLD_LOCAL_SIZE", -1);
    if (s > 0) {
        return s;
    }
    s = env_int("SLURM_NTASKS_PER_NODE", -1);
    if (s > 0) {
        return s;
    }
    // World size if MPI is up and we only know one process — last resort.
#if NRNMPI
    int flag = 0;
    if (MPI_Initialized(&flag) == MPI_SUCCESS && flag) {
        int world = 1;
        MPI_Comm_size(MPI_COMM_WORLD, &world);
        return world;
    }
#endif
    return 1;
}

std::atomic<int> g_assigned_device_id{-1};
std::atomic<bool> g_device_assigned{false};

/** Heuristic: CUDA Multi-Process Service control socket present. */
bool cuda_mps_likely_active() {
    char const* pipe_dir = std::getenv("CUDA_MPS_PIPE_DIRECTORY");
    if (!pipe_dir || !pipe_dir[0]) {
        pipe_dir = "/tmp/nvidia-mps";
    }
    std::string const control = std::string(pipe_dir) + "/control";
    return ::access(control.c_str(), F_OK) == 0;
}

}  // namespace

void assign_device() {
#if defined(NRN_ENABLE_GPU)
    if (g_device_assigned.exchange(true)) {
        return;
    }

    int num_devices_per_node = target_get_num_devices();
    if (num_devices_per_node == 0) {
        throw std::runtime_error(
            "neuron::gpu::assign_device: GPU execution enabled but no NVIDIA GPU found");
    }

    auto const requested = device_count();
    if (requested != 0) {
        if (static_cast<int>(requested) > num_devices_per_node) {
            throw std::runtime_error(
                "neuron::gpu::assign_device: requested device_count exceeds available GPUs");
        }
        num_devices_per_node = static_cast<int>(requested);
    }

    int const local_rank = mpi_local_rank();
    int const local_size = mpi_local_size();
    int const device_id = local_rank % num_devices_per_node;
    target_set_default_device(device_id);
    g_assigned_device_id.store(device_id);

    if (mpi_simulation_rank() == 0) {
        std::cout << " Info : " << num_devices_per_node << " GPUs shared by " << local_size
                  << " ranks per node\n";
        // Multi-process OpenACC without MPS thrashs on small kernels (dentate
        // 4-rank ~37 s vs ~3 s with MPS / 1-rank). CoreNEURON shares better
        // without MPS; native needs MPS for multi-rank on one GPU today.
        if (local_size > 1 && local_size > num_devices_per_node && !cuda_mps_likely_active()) {
            std::cerr
                << " Warning : " << local_size
                << " ranks share " << num_devices_per_node
                << " GPU(s) but CUDA MPS does not appear active. Native GPU multi-rank "
                   "on one device is often 10×+ slower without MPS. Start with:\n"
                   "   nvidia-cuda-mps-control -d\n"
                   " (see doc/gpu/native-coreneuron-parity.md multi-rank section)\n";
        }
    }
#else
    (void) 0;
#endif
}

int assigned_device_id() noexcept {
    return g_assigned_device_id.load();
}

namespace detail {

void reset_device_assignment_for_testing() {
    g_device_assigned.store(false);
    g_assigned_device_id.store(-1);
}

}  // namespace detail

}  // namespace neuron::gpu