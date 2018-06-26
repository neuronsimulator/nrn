from neuron import h, rxd, gui
import numpy
import sys
import time

npar = len(sys.argv)
#if(npar<2):
#    print "usage: python wave1d.py <nseg> <nsubseg>"
#    sys.exit(0)

#rxd.options.nsubseg =int(sys.argv[2])
rxd.options.subseg_interpolation = 0
rxd.options.subseg_averaging = 0
nsubseg = 5
rxd.set_solve_type(nsubseg=nsubseg)


def fill_section(dend):
    dend.L  = 100
    dend.diam = 1
    dend.nseg = 25
    dend.Ra = 150
    Rm =  25370
    for myseg in dend: myseg.v = -64  
    for myseg in dend: myseg.cm = 1.41
    dend.insert('pas')
    for myseg in dend: myseg.pas.g = 1.0/Rm
    for myseg in dend: myseg.pas.e = -64
    dend.insert('cal') # insert L-type Ca channel
    for myseg in dend: myseg.cal.gcalbar = 1.e-6


sec = h.Section()
sec2 = h.Section()
sec3 = h.Section()
sec4 = h.Section()
fill_section(sec)
fill_section(sec2)
fill_section(sec3)
fill_section(sec4)
sec2.connect(sec)
sec3.connect(sec)
sec4.connect(sec, .3, 0)

nstim = 5
st_dur= 2
st_interv = 50
st_start=1000
stims = []
for i in range(nstim):
    stim = h.IClamp(0.5, sec=sec)
    stim.delay=st_start+i*(st_interv)
    stim.dur = st_dur
    stim.amp=1.0/8000*1200
    stims.append(stim)

h.CVode().active(1)
h.cvode.atol(1e-6)

def set_plotshape_colormap(plotshape, cmap='jet'):
    import matplotlib.cm
    s = matplotlib.cm.ScalarMappable(cmap=cmap)
    cmap = s.get_cmap()
    s.set_clim(0, cmap.N)
    rs, gs, bs = zip(*s.to_rgba(range(cmap.N)))[0 : 3]
    plotshape.colormap(cmap.N)
    for i, r, g, b in zip(xrange(cmap.N), rs, gs, bs):
        plotshape.colormap(i, r * 255, g * 255, b * 255)
    # call s.scale(lo, hi) to replot the legend

s = h.PlotShape()

s.exec_menu('Shape Plot')
set_plotshape_colormap(s)

# show the diameters
s.show(0)

# cytoplasmic, er volume fractions
fc, fe = 0.83, 0.17

# parameters
caDiff = 0.016
#caDiff =0
ip3Diff = 0.283
#ip3Diff = 0
cac_init = 1.e-4
ip3_init = 0.1
gip3r = 12040
gserca = 0.3913
gleak = 6.020
kserca = 0.1
kip3 = 0.15
kact = 0.4
ip3rtau = 2000

# define the regions for the rxd
cyt = rxd.Region(h.allsec(), nrn_region='i', geometry=rxd.FractionalVolume(fc, surface_fraction=1))
er = rxd.Region(h.allsec(), geometry=rxd.FractionalVolume(fe))
cyt_er_membrane = rxd.Region(h.allsec(), geometry=rxd.FixedPerimeter(1))

# the species and other states
ca = rxd.Species([cyt, er], d=caDiff, name='ca', charge=2, initial=cac_init)
ip3 = rxd.Species(cyt, d=ip3Diff, initial=ip3_init)
ip3r_gate_state = rxd.State(cyt_er_membrane, initial=0.8)
h_gate = ip3r_gate_state[cyt_er_membrane]


# pumps and channels between ER and Cytosol

serca = rxd.MultiCompartmentReaction(ca[cyt]>ca[er], gserca/((kserca / (1000. * ca[cyt])) ** 2 + 1), membrane=cyt_er_membrane, custom_dynamics=True)
leak = rxd.MultiCompartmentReaction(ca[er]<>ca[cyt], gleak, gleak, membrane=cyt_er_membrane)

minf = ip3[cyt] * 1000. * ca[cyt] / (ip3[cyt] + kip3) / (1000. * ca[cyt] + kact)
k = gip3r * (minf * h_gate) ** 3 
ip3r = rxd.MultiCompartmentReaction(ca[er]<>ca[cyt], k, k, membrane=cyt_er_membrane)
ip3rg = rxd.Rate(h_gate, (1. / (1 + 1000. * ca[cyt] / (0.3)) - h_gate) / ip3rtau)

v1 = h.Vector()
v1.record(sec(0.5)._ref_v)
ca1 = h.Vector()
ca1.record(sec(0.5)._ref_cai)
v2 = h.Vector()
v2.record(sec(0.25)._ref_v)
ca2 = h.Vector()
ca2.record(sec(0.25)._ref_cai)
times = h.Vector()
times.record(h._ref_t)


h.finitialize()

cae_init = (0.0017 - cac_init * fc) / fe
ca[er].concentration = cae_init

#ip3.nodes.concentration = 2
for node in ip3.nodes:
    if  node.x<.8 and node.x>=.6  and node.sec._sec==sec:
        node.concentration = 2


h.CVode().re_init()

s.variable('cai')
#s.scale(-70, -50)
s.scale(0, 2e-3)

def dorec (dat, nl):
  i = 0 #to go over each node
  for node in nl:
    dat[i][datacol] = node.concentration
    i += 1


tstop=3000 
recdt = 100
datacol=0
data= numpy.zeros( (sec.nseg*nsubseg*4, numpy.ceil(tstop/recdt) ) )


# record cytoplasmic ca concentration
def dorecord ():
  global datacol
  dorec(data, ca[cyt].nodes)
  datacol = datacol+1


def save():
    print 't = %g, cai_mid = %g' % (h.t, sec.cai)
    s.flush()
    s.printfile('figs/wave1d_Clamp_%g.ps' % h.t)

def mydraw (data=data,vmin=[0,0,0],vmax=[0.002,0.2, 0.011],startt=0,endt=tstop,\
            miny=0,maxy=int(sec.L*4),keys=["cytca"],titles=["ca[cyt] (mM)","ip3[cyt] (mM)"],\
            interp='bilinear'):
  dat = data
  i=0
  mymin,mymax = numpy.amin(dat),numpy.amax(dat);
#  pyplot.imshow(dat, vmin=vmin[i], vmax=vmax[i],aspect='auto',extent=(0,tstop/1e3,0,int(sec.L*3)), origin='lower',interpolation=interp)
  pyplot.imshow(dat, vmin=mymin, vmax=mymax,aspect='auto',extent=(0,tstop/1e3,0,int(sec.L*4)), origin='lower',interpolation=interp)
  pyplot.xlim( (startt/1e3, endt/1e3) ); pyplot.ylim( (miny, maxy+1) );
  pyplot.ylabel(r'Position ($\mu$m)');  pyplot.title(titles[i]); pyplot.colorbar();
  pyplot.xlabel('Time(s)');


for t in xrange(0, tstop, recdt):
    h.CVode().event(t, save)
    h.CVode().event(t, dorecord)

try:
    import os
    os.mkdir('figs')
except OSError:
    # directory already exists
    pass

h.continuerun(tstop)

from matplotlib import pyplot
pyplot.subplot(2, 2, 1)
pyplot.plot(times.as_numpy(), v1.as_numpy())
pyplot.subplot(2, 2, 2)
pyplot.plot(times.as_numpy(), ca1.as_numpy())
pyplot.subplot(2, 2, 3)
pyplot.plot(times.as_numpy(), v2.as_numpy())
pyplot.subplot(2, 2, 4)
pyplot.plot(times.as_numpy(), ca2.as_numpy())
#pyplot.savefig('./gif/tree2_voltage_over_time'+str(sec.nseg)+"_"+str(nsubseg)+'.png')
pyplot.figure()
mydraw()
numpy.save("tree2ca_2d"+str(rxd.options.subseg_interpolation)+str(rxd.options.subseg_averaging)+"_nrxd"+str(sec.nseg)+"_"+str(nsubseg), data)
#pyplot.savefig('./gif/tree2_ca_2d'+str(sec.nseg)+"_"+str(nsubseg)+'.png')
