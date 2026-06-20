#include "neuron/gpu/device_assign.hpp"

#include "neuron/gpu/config.hpp"
#include "neuron/gpu/offload.hpp"

#include <atomic>
#include <iostream>
#include <stdexcept>

#if NRNMPI
#include <mpi.h>
extern int nrnmpi_use __attribute__((weak));
extern int nrnmpi_myid __attribute__((weak));
#endif

namespace neuron::gpu {
namespace {

int mpi_simulation_rank() {
#if NRNMPI
    if (&nrnmpi_myid != nullptr) {
        return nrnmpi_myid;
    }
#endif
    return 0;
}

int mpi_local_rank() {
#if NRNMPI
    if (&nrnmpi_use != nullptr && nrnmpi_use != 0) {
        MPI_Comm local_comm{};
        MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, mpi_simulation_rank(),
                            MPI_INFO_NULL, &local_comm);
        int local_rank = 0;
        MPI_Comm_rank(local_comm, &local_rank);
        MPI_Comm_free(&local_comm);
        return local_rank;
    }
#endif
    return 0;
}

int mpi_local_size() {
#if NRNMPI
    if (&nrnmpi_use != nullptr && nrnmpi_use != 0) {
        MPI_Comm local_comm{};
        MPI_Comm_split_type(MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, mpi_simulation_rank(),
                            MPI_INFO_NULL, &local_comm);
        int local_size = 1;
        MPI_Comm_size(local_comm, &local_size);
        MPI_Comm_free(&local_comm);
        return local_size;
    }
#endif
    return 1;
}

std::atomic<int> g_assigned_device_id{-1};
std::atomic<bool> g_device_assigned{false};

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
    int const device_id = local_rank % num_devices_per_node;
    target_set_default_device(device_id);
    g_assigned_device_id.store(device_id);

    if (mpi_simulation_rank() == 0) {
        std::cout << " Info : " << num_devices_per_node << " GPUs shared by "
                  << mpi_local_size() << " ranks per node\n";
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