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

import argparse, csv, itertools, numpy, h5py, json, sys
from collections import namedtuple

GidMap = namedtuple('GidMap','labels gids gid_class')
Spike = namedtuple('Spike','t gid')

def scan_gidmap(source):
    try:
        jmap = json.load(source)
        return GidMap(labels=jmap['labels'], gids=jmap['gids'],
                      gid_class=jmap['gid_class'])
    except ValueError:
        # try csv instead
        source.seek(0)
        gmap = { int(f[0]): f[1] for f in csv.reader(source) }
        gids = sorted(gmap.keys())
        labels = sorted(set(gmap.values()))
        label_lookup = { labels[i]: i for i in range(len(labels)) }
        gid_class = { g: label_lookup[gmap[g]] for g in gids }
        return GidMap(labels=labels, gids=gids, gid_class=gid_class)

# Split and sort spike times by gid class
def scan_spike_dat(source, gidmap=None):
    n_class = len(gidmap.labels) if gidmap else 1
    ts_set = [[] for i in range(n_class)]
    spikes = (Spike(t=float(y[0]),gid=int(y[1]))
              for y in [x.split() for x in source] if len(y)==2)

    raw = []
    for spike in spikes:
        raw.append(spike)
        try: 
            c = gidmap.gid_class[spike.gid] if gidmap else 0
            ts_set[c].append(spike.t)
        except KeyError:
            # missing keys => ignore this gid
            pass

    for c in range(n_class):
        ts_set[c].sort()

    if gidmap==None:
        uniq_gids = sorted(set((s.gid for s in raw)))
        gidmap = GidMap(labels=["all"], gids=uniq_gids, gid_class=[0 for g in uniq_gids])

    return ts_set,raw


# Like itertools.takewhile, but keep the tail
def splitafter(pred,it):
    front = []
    it = iter(it)
    for x in it:
        if pred(x):
            front.append(x)
        else:
            return iter(front), itertools.chain([x],it)
    return iter(front), iter([])

# return a list of sublists of the ordered sequence xs
# for each consecutive pair v0,v1 of elements in vs, containing those
# elements of xs within [v0,v1)
def split_ordseq_by(xs, vs, getx=lambda x: x):
    if not vs: return 

    xiter = itertools.dropwhile(lambda x: getx(x) < vs[0], xs)
    for v in vs[1:]:
        subseq, xiter = splitafter(lambda x: getx(x) < v, xiter)
        yield list(subseq)

# bins are [t0+i*dt,t0+(i+1)*dt) for i=0 to N-1 such that t0+N*dt<=t1 and t0+(N+1)*dt>t1

def bin_dt_count(t0,t1,dt):
    N = int((t1-t0)/dt);
    # adjust for possible precision loss
    if t0+N*dt <= t1:
        while t0+N*dt <= t1: N += 1
        N -= 1
    else:
        while t0+N*dt > t1: N -= 1
    return N

def bin_dt_gen(t0,t1,dt):
    N = bin_dt_count(t0,t1,dt)
    return (t0+i*dt for i in range(N))

def bin_dt(xs,t0,t1,dt):
    if t1 <= t0 or dt == 0:
        return numpy.array([])

    N = bin_dt_count(t0,t1,dt)
    binned = numpy.zeros(N)
    xi = 0
    while xi < len(xs) and xs[xi] < t0: xi += 1
    
    xj = xi
    for i in range(N):
        tr = t0+(i+1)*dt
        while xj < len(xs) and xs[xj] < tr: xj += 1
        binned[i] = xj-xi
        xi = xj

    return binned
    
# In hdf5 output, our dimension scales have a simple relationship with axes.

def create_and_attach_scale(ds,dim,label,scale_ds,scale_label=None):
    if scale_label==None: scale_label=label
    ds.dims[dim].label = label
    ds.dims.create_scale(scale_ds,scale_label)
    ds.dims[dim].attach_scale(scale_ds)

def floatlist(s):
    try:
        return sorted([float(x) for x in s.split(',')])
    except ValueError:
        raise argparse.ArgumentTypeError('{0} is not a comma-separated list of float'.format(s))

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('-i','--input', metavar='SPIKE.DAT',
                    required=True,
                    type=file,
                    help='Specify spike-time data file (mandatory)')
    ap.add_argument('--dt',
                    required=True,
                    type=float,
                    help='Aggregation time interval')
    ap.add_argument('-g','--gid-map', metavar='GID.CSV',
                    type=file,
                    help='Optionally sub-classify gids with gid to class map')
    ap.add_argument('-t','--t-range', metavar='T0,...,TN',
                    type=floatlist,
                    help='Optionally sub-divide spike data into N time intervals'\
                         '[T0,T1),[T1,T2),...,[T(N-1),TN)')
    ap.add_argument('hdf5_output',
                    help='Output HDF5 file')

    args = ap.parse_args()
    gid_map = None
    if args.gid_map:
        gid_map = scan_gidmap(args.gid_map)

    dt = args.dt;
    ts_set, spike_raw = scan_spike_dat(args.input,gid_map)
    spike_raw.sort(key=lambda s: s.t)

    # if we had no gidmap, construct from raw data

    if gid_map==None:
        uniq_gids = sorted(set((s.gid for s in spike_raw)))
        gid_map = GidMap(labels=["all"], gids=uniq_gids, gid_class=[0 for g in uniq_gids])

    n_gid = len(gid_map.gids)
    n_class = len(gid_map.labels)

    # split spike times by t-ranges

    t_range = args.t_range if args.t_range else [float('-inf'),float('inf')]
    n_tival = len(t_range)-1

    for c in range(n_class):
        ts_set[c] = list(split_ordseq_by(ts_set[c], t_range))

    # account for first spike per gid in t-range separately
    
    spike0 = []
    for spikes in split_ordseq_by(spike_raw, t_range, lambda s: s.t):
        seen_gid = set()
        spike0_i = []
        for spike in spikes:
            if spike.gid not in seen_gid:
                seen_gid.add(spike.gid)
                spike0_i.append(spike)
        spike0.append(spike0_i)

    # create HDF5 file

    try:
        H = h5py.File(args.hdf5_output, "w")
    except IOError:
        sys.exit("unable to open hdf5 file {0} for writing".format(args.hdf5_output))
        
    # save raw data

    ds = H.create_dataset('raw/spike_t', data=[s.t for s in spike_raw])
    ds.attrs['units'] = 'ms'

    ds = H.create_dataset('raw/spike_gid', data=[s.gid for s in spike_raw])
    create_and_attach_scale(ds, 0, 't', H['raw/spike_t'], 'spike time')

    # save classification scheme at top level
    
    dt_unicode = h5py.special_dtype(vlen = unicode)
    ds = H.create_dataset('gid_labels', (len(gid_map.labels),), dtype=dt_unicode)
    ds[:] = gid_map.labels;

    H.create_dataset('gids', data=gid_map.gids, dtype='i4')

    ds = H.create_dataset('gid_class', data=gid_map.gid_class, dtype='i4')
    create_and_attach_scale(ds, 0, 'gid', H['gids'])
    
    #for k in sorted(ts_set.keys()):
    #    print "class {0}:".format(k);
    #    for j in range(len(t_range)-1):
    #        print "interval [{0},{1}): {2}".format(t_range[j],t_range[j+1],len(ts_set[k][j]))
    
    # convert to sampled spike counts in binned_set, and power spectrum into ps_set

    t_min = min((s.t for s in spike_raw))
    t_max = max((s.t for s in spike_raw))

    # make and store time sub-intervals

    t_interval = []
    for j in range(n_tival):
        t0 = t_range[j]
        if t0 == float('-inf'): t0 = t_min

        t1 = t_range[j+1]
        if t1 == float('inf'): t1 = t_max+dt

        N = bin_dt_count(t0,t1,dt)
        t_interval.append((t0,t1))

        group = 'span/{0}/'.format(j)
        
        ds = H.create_dataset(group+'t_range', data=[t0,t1])
        ds.attrs['units'] = 'ms'

        ds = H.create_dataset(group+'t', data=list(bin_dt_gen(t0,t1,dt)))
        ds.attrs['units'] = 'ms'
        ds.attrs['dt'] = dt

        M = 1+(N/2)
        ds = H.create_dataset(group+'f', data=[k/(t1-t0) for k in range(M)])
        ds.attrs['units'] = 'kHz'
        ds.attrs['df'] = 1/(t1-t0)

        ds = H.create_dataset(group+'count', (n_class,), dtype='i8')
        create_and_attach_scale(ds, 0, 'class', H['gid_labels'])
        count_ds = ds

        ds = H.create_dataset(group+'spike0/t', data=list(s.t for s in spike0[j]))
        ds.attrs['units'] = 'ms'

        ds = H.create_dataset(group+'spike0/gid', data=list(s.gid for s in spike0[j]), dtype='i4')
        create_and_attach_scale(ds, 0, 't', H[group+'spike0/t'])

        ds = H.create_dataset(group+'sample',(n_class,N))
        create_and_attach_scale(ds, 0, 'class', H['gid_labels'])
        create_and_attach_scale(ds, 1, 't', H[group+'t'], 't bin start')
        sample_ds = ds

        ds = H.create_dataset(group+'power_spectrum',(n_class,M))
        create_and_attach_scale(ds, 0, 'class', H['gid_labels'])
        create_and_attach_scale(ds, 1, 'f', H[group+'f'], 'frequency')
        pspectrum_ds = ds

        # sample and fft sub-interval data by class

        for k in range(n_class):
            count_ds[k] = len(ts_set[k][j])
            samples = bin_dt(ts_set[k][j],t0,t1,dt)
            sample_ds[k,:] = samples
            pspectrum_ds[k,:] = abs(numpy.fft.rfft(samples))**2
            
        
if __name__ == '__main__':
    main()
