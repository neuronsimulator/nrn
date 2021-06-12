import sys
from neuron import h

h.nrnmpi_init()
pc = h.ParallelContext()

id = int(pc.id())
nhost = int(pc.nhost())

if "--expected-hosts" in sys.argv:
    expected_nhost_index = sys.argv.index("--expected-hosts") + 1
    expected_nhost = int(sys.argv[expected_nhost_index])
else:
    # if there is no cli arg then accept nhost as expected host count
    expected_nhost = nhost

print("I am %d of %d" % (id, nhost))

if nhost != expected_nhost:
    sys.exit(
        "MPI Error : #ranks ({}) different that the expected ({})".format(
            nhost, expected_nhost
        )
    )

pc.barrier()
h.quit()
