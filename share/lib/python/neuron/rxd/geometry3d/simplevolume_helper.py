from . import graphicsPrimitives as graphics
import math
import random

def get_verts(voxel,g):
    """return list (len=8) of point coordinates (x,y,z) that are vertices of the voxel (i,j,k)"""
    (i,j,k) = voxel
    dx,dy,dz = g['dx'],g['dy'],g['dz']
    v1_0,v1_1,v1_2 = g['xlo'] + i*dx, g['ylo'] + j*dy, g['zlo'] + k*dz
    vertices = [(v1_0,v1_1,v1_2), 
                (v1_0+dx,v1_1,v1_2),
                (v1_0+dx,v1_1+dy,v1_2),
                (v1_0,v1_1+dy,v1_2),
                (v1_0,v1_1,v1_2+dz),
                (v1_0+dx,v1_1,v1_2+dz),
                (v1_0+dx,v1_1+dy,v1_2+dz),
                (v1_0,v1_1+dy,v1_2+dz)]
    return vertices 

def get_subverts(voxel,g,step):
    """return list (len=8) of point coordinates (x,y,z) that are vertices of the voxel (i,j,k)"""
    (i,j,k) = voxel
    dx,dy,dz = g['dx'],g['dy'],g['dz']
    DX, DY, DZ = step
    v1_0,v1_1,v1_2 = g['xlo'] + i*dx, g['ylo'] + j*dy, g['zlo'] + k*dz
    vertices = [(v1_0,v1_1,v1_2), 
                (v1_0+DX,v1_1,v1_2),
                (v1_0+DX,v1_1+DY,v1_2),
                (v1_0,v1_1+DY,v1_2),
                (v1_0,v1_1,v1_2+DZ),
                (v1_0+DX,v1_1,v1_2+DZ),
                (v1_0+DX,v1_1+DY,v1_2+DZ),
                (v1_0,v1_1+DY,v1_2+DZ)]
    return vertices 

def add_res(flist, voxel, verts_in, res, g):        
    (i, j, k) = voxel
    dx,dy,dz = g['dx'],g['dy'],g['dz']
    bit = (dx * dy * dz)/(res**3)
    
    step = [dx/2, dy/2, dz/2]
    subverts = get_subverts(voxel, g, step)               # the 'voxel positions' for the new subvoxels
    
    #verts_in = [0,1,2,3,4,5,6,7]
    count = 0
    #select only the subvoxels of the vertices that are in
    for i in verts_in:
        v0 = subverts[i]

        startpt = (v0[0]+dx/(2*res), v0[1]+dy/(2*res), v0[2]+dx/(2*res))
        for x in range(res//2):
            for y in range(res//2):
                for z in range(res//2):
                    v = (startpt[0] + x*(dx/res), startpt[1] + y*(dy/res), startpt[2] + z*(dz/res))
                    if  min([f.distance(v[0],v[1],v[2]) for f in flist]) <= 0:
                        count += 1
    
    vol = count*bit 
    # add in partials if still 0
    if vol == 0:
        vol = bit/2
    
    return vol

def Put(flist, voxel, v0, verts_in, res, g):
    """ add voxel key with partial volume value to dict of surface voxels"""
    # v0 is the start coordinates of voxel (verts[0] feed in)
    # res is resolution of sampling points (only works for even values of res!)
    dx,dy,dz = g['dx'],g['dy'],g['dz']
    bit = (dx * dy * dz)/(res**3)
    
    count = 0
    startpt = (v0[0]+dx/(2*res), v0[1]+dy/(2*res), v0[2]+dx/(2*res))
    for x in range(res):
        for y in range(res):
            for z in range(res):
                v = (startpt[0] + x*(dx/res), startpt[1] + y*(dy/res), startpt[2] + z*(dz/res))
                if min([f.distance(v[0],v[1],v[2]) for f in flist]) <= 0:
                    count += 1
    bitvol = count*bit             
    if bitvol == 0:
        bitvol = add_res(flist, voxel, verts_in, res*2, g)
           
    return bitvol

def simplevolume(flist,distances,voxel,g):
    """return the number of vertices of this voxel that are contained within the surface"""
    verts = get_verts(voxel,g)
    verts_in = []
    for i in range(8):
        if distances[i] <= 0:
            verts_in.append(i)
    Vol = Put(flist, voxel, verts[0], verts_in, 2, g)
    return Vol














