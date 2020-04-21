import sys
from neuron import h

if "--expected-hosts" in sys.argv:
    expected_host_index = sys.argv.index("--expected-hosts") + 1
    expected_hosts = int(sys.argv[expected_host_index])
else:
    expected_hosts = 0

h.nrnmpi_init()
pc = h.ParallelContext()

id = int(pc.id())
nhost = int(pc.nhost())

if expected_hosts & (nhost != expected_hosts):
    sys.exit("Number of ranks ({}) different that the expected ({})".format(nhost, expected_hosts))

print ('I am %d of %d'%(id, nhost))

pc.barrier()
h.quit()
