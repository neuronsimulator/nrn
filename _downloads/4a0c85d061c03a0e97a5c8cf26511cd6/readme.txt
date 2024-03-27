ipulse1.mod
NMODL specification for Ipulse1.

ipulse2.mod
NMODL specification for Ipulse2.

These Point Processes are current clamps for delivering one or 
more current pulses at regular intervals.  Both use self-events 
to govern switching the current on and off.  Although these 
mechanisms have NET_RECEIVE blocks, they will not respond to 
external events, so it won't do any good to try to attach 
them to an event source such as a NetStim.

The chief difference between these mechanisms is how one 
specifies the stimulus train.  Ipulse1 needs ton (duration of 
current pulse) and toff (interpulse interval), 
while Ipulse2 needs dur (same as Ipulse1's ton) and per 
(the period of the stimlus train, which is the same as 
Ipulse1's ton+toff).  A bit of code in ipulse2.mod forces 
per to be longer than dur.

ipulse3.mod
NMODL specification for Ipulse3.

Delivery of an input event (i.e. via a NetCon) triggers a
single current pulse of user-specified amplitude and duration.
Input events that arrive during an ongoing pulse are ignored.

test_1_and_2.hoc
A GUI-constructed experimental rig for testing Ipulse1 and Ipulse2.

test_3.hoc
Tests Ipulse3.
