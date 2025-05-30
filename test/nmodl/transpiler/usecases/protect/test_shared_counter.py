from neuron import h, gui


def test_shared_counter():
    nthreads = 32
    nseg = 10
    nsteps = 100

    sections = [h.Section() for _ in range(nthreads)]
    for s in sections:
        s.insert("shared_counter")
        s.nseg = nseg

    pc = h.ParallelContext()

    pc.nthread(nthreads)
    for k, s in enumerate(sections):
        pc.partition(k, h.SectionList([s]))

    h.finitialize()
    for _ in range(nsteps):
        h.step()

    expected = nthreads * nseg * nsteps
    g_cnt = h.g_cnt_shared_counter
    assert h.g_cnt_shared_counter == expected, f"{g_cnt}"


if __name__ == "__main__":
    test_shared_counter()
