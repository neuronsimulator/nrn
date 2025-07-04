from neuron import h, gui, coreneuron
from pathlib import Path
from typing import List


def inspect(v):
    print(v, type(v), id(v))
    for i in dir(v):
        print(i)

class Cell:
    def __init__(self, gid):
        self.gid = gid
        self.soma = h.Section(name="soma", cell=self)
        self.soma.nseg = 1
        self.soma.L = 5.6419
        self.soma.diam = 5.6419
        self.soma.insert("hh")
        self.ic = h.IClamp(self.soma(0.5))
        self.ic.delay = 0.5
        self.ic.dur = 0.1
        self.ic.amp = 0.3
        self.soma.insert("pas")
        self.soma(0.5).pas.g = 0.001
        self.soma(0.5).pas.e = -65
        # register gid
        pc = h.ParallelContext()
        # set netcon to soma
        nc = h.NetCon(self.soma(0.5)._ref_v, None, sec=self.soma)
        # assign gid to this rank
        pc.set_gid2node(self.gid, pc.id())
        # associate gid to the netcon
        pc.cell(self.gid, nc, 0)


# function to register section-segment mapping with bbcore write
def setup_nrnbbcore_register_mapping(gid):

    #for recording
    recordlist = []

    pc = h.ParallelContext()

    #vector for soma sections and segment
    somasec = h.Vector()
    somaseg = h.Vector()

    #vector for dendrite sections and segment
    densec = h.Vector()
    denseg = h.Vector()

    #if gid exist on rank
    if (pc.gid_exists(gid)):

        #get cell instance
        cell = pc.gid2cell(gid)
        isec = 0

        #soma section, only pne
        for sec in [cell.soma]:
            for seg in sec:
                #get section and segment index
                somasec.append(isec)
                somaseg.append(seg.node_index())

                #vector for recording
                v = h.Vector()
                v.record(seg._ref_v)
                v.label("soma %d %d" % (isec, seg.node_index()))
                recordlist.append(v)
        isec += 1

        #register soma section list
        pc.nrnbbcore_register_mapping(gid, "soma", somasec, somaseg)

    return recordlist

def write_report_config(output_file, report_name, target_name, report_type, report_variable,
                        unit, report_format, target_type, dt, start_time, end_time, gids,
                        buffer_size=8):
    import struct
    num_gids = len(gids)
    report_conf = Path(output_file)
    report_conf.parent.mkdir(parents=True, exist_ok=True)
    with report_conf.open("wb") as fp:
        # Write the formatted string to the file
        fp.write(b"1\n")
        fp.write(("%s %s %s %s %s %s %d %lf %lf %lf %d %d\n" % (
            report_name,
            target_name,
            report_type,
            report_variable,
            unit,
            report_format,
            target_type,
            dt,
            start_time,
            end_time,
            num_gids,
            buffer_size
        )).encode())
        # Write the array of integers to the file in binary format
        fp.write(struct.pack(f'{num_gids}i', *gids))
        fp.write(b'\n')

def write_spike_config(output_file: str, spike_filename: str,
                       population_names: List[str], population_offsets: List[int]):
    report_conf = Path(output_file)
    num_population = len(population_names)
    with report_conf.open("a") as fp:
        fp.write(f"{num_population}\n")
        for pop_name, offset in zip(population_names, population_offsets):
            fp.write(f"{pop_name} {offset}\n")
        fp.write(f"{spike_filename}\n")

def write_sim_config(output_file, coredata_dir, report_conf, tstop):
    sim_conf = Path(output_file)
    sim_conf.parent.mkdir(parents=True, exist_ok=True)
    Path(coredata_dir).mkdir(parents=True, exist_ok=True)
    with sim_conf.open("w") as fp:
        fp.write("outpath=./\n")
        fp.write(f"datpath=./{coredata_dir}\n")
        fp.write(f"tstop={tstop}\n")
        fp.write(f"report-conf='{report_conf}'\n")
        fp.write("mpi=true\n")

def spike_record():
    global tvec, idvec
    pc = h.ParallelContext()
    tvec = h.Vector(1000000)
    idvec = h.Vector(1000000)
    pc.spike_record(-1, tvec, idvec)

def test_coreneuron_report():
    # model setup
    pc = h.ParallelContext()

    c = Cell(0)
    h.cvode.use_fast_imem(1)

    # activate coreneuron
    coreneuron.enable = True
    coreneuron.file_mode = False
    coreneuron.gpu = False
    coreneuron.cell_permute = False

    # register reports
    if pc.id() == 0:
        setup_nrnbbcore_register_mapping(c.gid)
        report_conf_file = "report.conf"
        sim_conf_file = "sim.conf"
        write_report_config(report_conf_file, "soma_v.h5", "Mosaic", "compartment", "v",
                    "mV", "SONATA", 2, 1, 0, h.tstop, [c.gid])
        write_spike_config(report_conf_file, "spikes.h5", ["default"], [0])
        write_sim_config(sim_conf_file, "corenrn_data", report_conf_file, h.tstop)
    coreneuron.sim_config=sim_conf_file

    spike_record()

    # run
    pc.set_maxstep(10)
    h.stdinit()
    pc.psolve(h.tstop)



    # assert False


    


    
    
#     assert False

# from neuron import h, gui
# from neuron.units import ms, mV

# class Cell:
#     def __init__(self, gid, x, y, z, theta):
#         self._gid = gid
#         self._setup_morphology()
#         self.all = self.soma.wholetree()
#         self._setup_biophysics()
#         self.x = self.y = self.z = 0
#         h.define_shape()
#         self._rotate_z(theta)
#         self._set_position(x, y, z)

#         # everything below here in this method is NEW
#         self._spike_detector = h.NetCon(self.soma(0.5)._ref_v, None, sec=self.soma)
#         self.spike_times = h.Vector()
#         self._spike_detector.record(self.spike_times)

#         self._ncs = []

#         self.soma_v = h.Vector().record(self.soma(0.5)._ref_v)

#     def __repr__(self):
#         return "{}[{}]".format(self.name, self._gid)

#     def _set_position(self, x, y, z):
#         for sec in self.all:
#             for i in range(sec.n3d()):
#                 sec.pt3dchange(
#                     i,
#                     x - self.x + sec.x3d(i),
#                     y - self.y + sec.y3d(i),
#                     z - self.z + sec.z3d(i),
#                     sec.diam3d(i),
#                 )
#         self.x, self.y, self.z = x, y, z

#     def _rotate_z(self, theta):
#         """Rotate the cell about the Z axis."""
#         for sec in self.all:
#             for i in range(sec.n3d()):
#                 x = sec.x3d(i)
#                 y = sec.y3d(i)
#                 c = h.cos(theta)
#                 s = h.sin(theta)
#                 xprime = x * c - y * s
#                 yprime = x * s + y * c
#                 sec.pt3dchange(i, xprime, yprime, sec.z3d(i), sec.diam3d(i))

# class BallAndStick(Cell):
#     name = "BallAndStick"

#     def _setup_morphology(self):
#         self.soma = h.Section(name="soma", cell=self)
#         self.dend = h.Section(name="dend", cell=self)
#         self.dend.connect(self.soma)
#         self.soma.L = self.soma.diam = 12.6157
#         self.dend.L = 200
#         self.dend.diam = 1

#     def _setup_biophysics(self):
#         for sec in self.all:
#             sec.Ra = 100  # Axial resistance in Ohm * cm
#             sec.cm = 1  # Membrane capacitance in micro Farads / cm^2
#         self.soma.insert("hh")
#         self.soma.insert("pas")
#         for seg in self.soma:
#             seg.hh.gnabar = 0.12  # Sodium conductance in S/cm2
#             seg.hh.gkbar = 0.036  # Potassium conductance in S/cm2
#             seg.hh.gl = 0.0003  # Leak conductance in S/cm2
#             seg.hh.el = -54.3  # Reversal potential in mV
#             seg.pas.g = 0.001  # Passive conductance in S/cm2
#             seg.pas.e = -65  # Leak reversal potential mV
#         # Insert passive current in the dendrite
#         self.dend.insert("pas")
#         for seg in self.dend:
#             seg.pas.g = 0.001  # Passive conductance in S/cm2
#             seg.pas.e = -65  # Leak reversal potential mV

#         # NEW: the synapse
#         self.syn = h.ExpSyn(self.dend(0.5))
#         self.syn.tau = 2 * ms

# class Ring:
#     """A network of *N* ball-and-stick cells where cell n makes an
#     excitatory synapse onto cell n + 1 and the last, Nth cell in the
#     network projects to the first cell.
#     """

#     def __init__(
#         self, N=5, stim_w=0.04, stim_t=9, stim_delay=1, syn_w=0.01, syn_delay=5, r=50
#     ):
#         """
#         :param N: Number of cells.
#         :param stim_w: Weight of the stimulus
#         :param stim_t: time of the stimulus (in ms)
#         :param stim_delay: delay of the stimulus (in ms)
#         :param syn_w: Synaptic weight
#         :param syn_delay: Delay of the synapse
#         :param r: radius of the network
#         """
#         self._syn_w = syn_w
#         self._syn_delay = syn_delay
#         self._create_cells(N, r)
#         self._connect_cells()
#         # add stimulus
#         self._netstim = h.NetStim()
#         self._netstim.number = 1
#         self._netstim.start = stim_t
#         self._nc = h.NetCon(self._netstim, self.cells[0].syn)
#         self._nc.delay = stim_delay
#         self._nc.weight[0] = stim_w

#     def _create_cells(self, N, r):
#         self.cells = []
#         for i in range(N):
#             theta = i * 2 * h.PI / N
#             self.cells.append(
#                 BallAndStick(i, h.cos(theta) * r, h.sin(theta) * r, 0, theta)
#             )

#     def _connect_cells(self):
#         for source, target in zip(self.cells, self.cells[1:] + [self.cells[0]]):
#             nc = h.NetCon(source.soma(0.5)._ref_v, target.syn, sec=source.soma)
#             nc.weight[0] = self._syn_w
#             nc.delay = self._syn_delay
#             source._ncs.append(nc)

# def test_bau():
#     ring = Ring(N=5)
#     pc = h.ParallelContext()
#     for i in range(10):
#         print(pc.gid_exists(i))
    
#     assert False

# # from neuron.tests.utils.strtobool import strtobool
# # import os

# # from pathlib import Path
# # from typing import List

# # from neuron import h, gui, coreneuron
# # from neuron.units import ms, mV, µm



# # def write_report_config(output_file, report_name, target_name, report_type, report_variable,
# #                         unit, report_format, target_type, dt, start_time, end_time, gids,
# #                         buffer_size=8):
# #     import struct
# #     num_gids = len(gids)
# #     report_conf = Path(output_file)
# #     report_conf.parent.mkdir(parents=True, exist_ok=True)
# #     with report_conf.open("wb") as fp:
# #         # Write the formatted string to the file
# #         fp.write(b"1\n")
# #         fp.write(("%s %s %s %s %s %s %d %lf %lf %lf %d %d\n" % (
# #             report_name,
# #             target_name,
# #             report_type,
# #             report_variable,
# #             unit,
# #             report_format,
# #             target_type,
# #             dt,
# #             start_time,
# #             end_time,
# #             num_gids,
# #             buffer_size
# #         )).encode())
# #         # Write the array of integers to the file in binary format
# #         fp.write(struct.pack(f'{num_gids}i', *gids))
# #         fp.write(b'\n')

# # def write_spike_config(output_file: str, spike_filename: str,
# #                        population_names: List[str], population_offsets: List[int]):
# #     report_conf = Path(output_file)
# #     num_population = len(population_names)
# #     with report_conf.open("a") as fp:
# #         fp.write(f"{num_population}\n")
# #         for pop_name, offset in zip(population_names, population_offsets):
# #             fp.write(f"{pop_name} {offset}\n")
# #         fp.write(f"{spike_filename}\n")

# # def write_sim_config(output_file, coredata_dir, report_conf, tstop):
# #     sim_conf = Path(output_file)
# #     sim_conf.parent.mkdir(parents=True, exist_ok=True)
# #     os.makedirs(coredata_dir, exist_ok=True)
# #     with sim_conf.open("w") as fp:
# #         fp.write("outpath=./\n")
# #         fp.write(f"datpath=./{coredata_dir}\n")
# #         fp.write(f"tstop={tstop}\n")
# #         fp.write(f"report-conf='{report_conf}'\n")
# #         fp.write("mpi=true\n")


# # # function to register section-segment mapping with bbcore write
# # def setup_nrnbbcore_register_mapping(cell):

# #     #for recording
# #     recordlist = []

# #     # pc = h.ParallelContext()

# #     # #vector for soma sections and segment
# #     # somasec = h.Vector()
# #     # somaseg = h.Vector()
# #     # isec = 0

# #     # #     #soma section, only pne
# #     # for sec in [cell.soma]:
# #     #     for seg in sec:
# #     #         #get section and segment index
# #     #         somasec.append(isec)
# #     #         somaseg.append(seg.node_index())

# #     #         #vector for recording
# #     #         v = h.Vector()
# #     #         v.record(seg._ref_v)
# #     #         v.label("soma %d %d" % (isec, seg.node_index()))
# #     #         recordlist.append(v)
# #     #     isec += 1

# #     # #     #register soma section list
# #     # pc.nrnbbcore_register_mapping(0, "soma", somasec, somaseg)

# #     return recordlist

# # def set_reports(c):
# #     setup_nrnbbcore_register_mapping(c)

# #     report_conf_file = "report.conf"
# #     sim_conf_file = "sim.conf"


# #     write_report_config(output_file=report_conf_file, 
# #                         report_name="soma.h5", 
# #                         target_name="Mosaic",
# #                         report_type="compartment",
# #                         report_variable="v",
# #                         unit="mV", report_format="SONATA", target_type=2, dt=1, start_time=0, end_time=h.tstop, gids=[0])
# #     write_sim_config(sim_conf_file, "corenrn_data", report_conf_file, h.tstop)
# #     # coreneuron.sim_config=sim_conf_file
# #     # pc = h.ParallelContext()
# #     # pc.set_gid2node(0, 0)


# # class Ball:
# #     def __init__(self, gid):
# #         self._gid = gid
# #         self.soma = h.Section(name="soma", cell=self)
# #         self.soma.L = 5.6419
# #         self.soma.diam = 5.6419
# #         self.soma.insert("hh")
# #         self.ic = h.IClamp(self.soma(0.5))
# #         self.ic.delay = 0.5
# #         self.ic.dur = 0.1
# #         self.ic.amp = 0.3
# #         self.soma.insert("pas")  
# #         h.cvode.use_fast_imem(1)

# # def get_cell():
# #     c = Ball(0)
# #     return c
    

    

# # #     v = h.Vector()
# # #     v.record(h.soma(0.5)._ref_v, sec=h.soma)
# # #     i_mem = h.Vector()
# # #     i_mem.record(h.soma(0.5)._ref_i_membrane_, sec=h.soma)
# # #     i_pas = h.Vector()
# # #     i_pas.record(h.soma(0.5).pas._ref_i, sec=h.soma)

# # #     pc = h.ParallelContext()



# # def run():
# #     coreneuron.enable = True
# #     coreneuron.verbose = 0
# #     coreneuron.model_stats = True
# #     coreneuron.gpu = bool(strtobool(os.environ.get("CORENRN_ENABLE_GPU", "false")))
# #     coreneuron.num_gpus = 1

# #     pc = h.ParallelContext()
# #     pc.set_maxstep(10)
# #     h.stdinit()
# #     pc.psolve(h.tstop)

# #     # cnargs = coreneuron.nrncore_arg(h.tstop)
# #     # h.stdinit()
# #     # pc.nrncore_run(cnargs, 1)

# # def test_bau():
# #     c = get_cell()
# #     set_reports(c)
# #     run()



