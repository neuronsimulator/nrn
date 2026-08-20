HOC tests that were moved from https://github.com/neuronsimulator/nrntest/tree/a85864d1b2a7a531716e3c4908dec83faad83020/fast

Name | Old Name
basic | t1
load-neurondemo | t2
fixed-step-integrator | t3
multisplit | t4
multithreading | t5
variable-step-integrator | t6
variable-step-integrator-with-multisplit-multithread | t7
daspk-solver | t8
nmodl-before-after | t9
nmodl-before-after-cvode-daspk | t10
nmodl-before-after-multithreading | t11
nmodl-watch | t12
for-netcons | t15
longitudinal-diffusion | t16
vector | t17
ring6 | t18
savestate-ring6 | t19
savestate-single-compartment-HH | t20
connect_dend | connect_dend
ringtest | ringtest

Note that t13 and t14 were missing

The mod files required for the test are compiled once, so they have to exist in the same directory:
nadifl.mod
nmodl_watch.mod
stdp.mod
t9.mod

ring6.ses
