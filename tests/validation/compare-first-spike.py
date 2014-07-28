Copyright (c) 2014 EPFL-BBP, All rights reserved.

THIS SOFTWARE IS PROVIDED BY THE BLUE BRAIN PROJECT ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE BLUE BRAIN PROJECT
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#!/usr/bin/python

import argparse, itertools, h5py
from math import sqrt
from collections import namedtuple

Spike = namedtuple('Spike','t gid')

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('-i','--input', metavar='SPIKESCAN.H5',
                    required=True,
                    help='Specify spike-time data file (mandatory)')
    ap.add_argument('-r','--reference', metavar='REFERENCE_SPIKESCAN.H5',
                    required=True,
                    help='Specify reference spike-time data file (mandatory)')

    args = ap.parse_args()

    # create HDF5 file
    try:
        Hi = h5py.File(args.input, "r")
    except IOError:
        sys.exit("unable to open hdf5 file {0} for reading".format(args.input))
    # create HDF5 file
    try:
        Hr = h5py.File(args.reference, "r")
    except IOError:
        sys.exit("unable to open hdf5 file {0} for reading".format(args.reference))

    t0 = Hi['span/0/spike0/t']
    gids0 = Hi['span/0/spike0/gid']
    Rt0 = Hr['span/0/spike0/t']
    Rgids0 = Hr['span/0/spike0/gid']

    dset = Hi['span/0/t']
    dt = dset.attrs['dt']

#    Ginputspikes = (Spike(t=t0[i] , gid=gids0[i]) for i in range(len(t0)) )
#    Grefspikes = (Spike(t=Rt0[i] , gid=Rgids0[i]) for i in range(len(Rt0)) )

    inputspikes = []
    refspikes = []

    for i in range(len(t0)):
        inputspikes.append( Spike(t=t0[i] , gid=gids0[i])  )
    for i in range(len(Rt0)):
        refspikes.append( Spike(t=Rt0[i] , gid=Rgids0[i])  )

    inputspikes.sort(key=lambda s:s.gid)
    refspikes.sort(key=lambda s:s.gid)

    NonMatchingSpikes = 0
    MatchingSpikes = 0
    RMS = 0
    j = 0
    i = 0

    while i < len(inputspikes) and j < len(refspikes):
        if inputspikes[i].gid == refspikes[j].gid:
            RMS = RMS + (inputspikes[i].t - refspikes[j].t)**2
            i = i+1
            j = j+1
            MatchingSpikes = MatchingSpikes + 1
        elif inputspikes[i].gid < refspikes[j].gid:
            NonMatchingSpikes = NonMatchingSpikes + 1
            i = i + 1
        else:
            NonMatchingSpikes = NonMatchingSpikes + 1
            j = j + 1

    NonMatchingSpikes = NonMatchingSpikes + len(inputspikes) - i + len(refspikes) - j
    RMS = sqrt(RMS/MatchingSpikes)

    tol = sqrt(0.01)*dt
    passflag = 1
    if RMS > tol:
        print "******************************************************************"
        print "Test not passed: Root Mean Square of spike time-shift is too large"
        print "Computed value: " + str(RMS)
        print "tolerance: " + str(tol)
        print "******************************************************************"
        passflag = 0
    if NonMatchingSpikes > 0.1*MatchingSpikes:
        print "******************************************************************"
        print "Test not passed: too many mismatches between computed and reference spikes"
        print "Number of mismatches :" + str(NonMatchingSpikes)
        print "******************************************************************"
        print "tolerance: " + str(0.1*MatchingSpikes)
        passflag = 0
    if passflag:
        print "******************************************************************"
        print "Test Passed"
        print "******************************************************************"

if __name__ == '__main__':
    main()
