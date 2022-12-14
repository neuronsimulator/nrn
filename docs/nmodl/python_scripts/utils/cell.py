from neuron import h


class Cell:

    section = None
    record_vectors = {}

    def __init__(self):
        self._create_cell()

    def _create_cell(self):
        pass

    def create_circuit(self):
        pass

    def record(self):
        pass

    def simulate(self, tstop, dt):
        h.dt = dt
        h.finitialize()
        while h.t < tstop - dt / 2:
            h.fadvance()

    def output(self):
        for variable in self.record_vectors:
            print("Values of variable {}:".format(variable))
            values = self.record_vectors[variable]
            for i in range(0, len(values)):
                print("{} {}".format(i, values[i]))

    def __del__(self):
        self.record_vectors.clear()
