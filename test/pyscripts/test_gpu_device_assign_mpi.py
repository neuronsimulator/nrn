"""MPI smoke test for native GPU device assignment (local_rank % num_gpus)."""
from neuron import gpu, h


def main():
    pc = h.ParallelContext()
    nhost = int(pc.nhost())
    rank = int(pc.id())

    gpu.enable = True
    gpu.backend = "native"
    gpu.device_count = 0
    pc.gpu_assign_device()

    device_id = int(pc.gpu_device_id())
    assert device_id >= 0, "device_id should be assigned on rank {}".format(rank)

    print("rank={} nhost={} gpu_device_id={}".format(rank, nhost, device_id))
    assert nhost >= 2, "expected MPI with at least 2 ranks, got {}".format(nhost)


if __name__ == "__main__":
    main()