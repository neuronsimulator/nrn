from . import graphicsPrimitives as graphics
from .. import options
import math
import random


def get_verts(voxel, g):
    """return list (len=8) of point coordinates (x,y,z) that are vertices of the voxel (i,j,k)"""
    (i, j, k) = voxel
    dx, dy, dz = g["dx"], g["dy"], g["dz"]
    v1_0, v1_1, v1_2 = g["xlo"] + i * dx, g["ylo"] + j * dy, g["zlo"] + k * dz
    vertices = [
        (v1_0, v1_1, v1_2),
        (v1_0 + dx, v1_1, v1_2),
        (v1_0 + dx, v1_1 + dy, v1_2),
        (v1_0, v1_1 + dy, v1_2),
        (v1_0, v1_1, v1_2 + dz),
        (v1_0 + dx, v1_1, v1_2 + dz),
        (v1_0 + dx, v1_1 + dy, v1_2 + dz),
        (v1_0, v1_1 + dy, v1_2 + dz),
    ]
    return vertices


def get_subverts(voxel, g, step):
    """return list (len=8) of point coordinates (x,y,z) that are vertices of the voxel (i,j,k)"""
    (i, j, k) = voxel
    dx, dy, dz = g["dx"], g["dy"], g["dz"]
    DX, DY, DZ = step
    v1_0, v1_1, v1_2 = g["xlo"] + i * dx, g["ylo"] + j * dy, g["zlo"] + k * dz
    vertices = [
        (v1_0, v1_1, v1_2),
        (v1_0 + DX, v1_1, v1_2),
        (v1_0 + DX, v1_1 + DY, v1_2),
        (v1_0, v1_1 + DY, v1_2),
        (v1_0, v1_1, v1_2 + DZ),
        (v1_0 + DX, v1_1, v1_2 + DZ),
        (v1_0 + DX, v1_1 + DY, v1_2 + DZ),
        (v1_0, v1_1 + DY, v1_2 + DZ),
    ]
    return vertices


def add_res(flist, voxel, verts_in, res, g):
    dx, dy, dz = g["dx"], g["dy"], g["dz"]
    Sx, Sy, Sz = dx / res, dy / res, dz / res
    bit = Sx * Sy * Sz

    step = [dx / 2, dy / 2, dz / 2]
    subverts = get_subverts(
        voxel, g, step
    )  # the 'voxel positions' for the new subvoxels

    count = 0
    # select only the subvoxels of the vertices that are in
    for i in verts_in:
        v0 = subverts[i]
        startpt = (
            v0[0] + dx / (2 * res),
            v0[1] + dy / (2 * res),
            v0[2] + dx / (2 * res),
        )
        for x in range(res // 2):
            for y in range(res // 2):
                for z in range(res // 2):
                    v = (
                        startpt[0] + x * (dx / res),
                        startpt[1] + y * (dy / res),
                        startpt[2] + z * (dz / res),
                    )
                    if any(
                        f.distance(v[0], v[1], v[2]) <= options.ics_distance_threshold
                        for f in flist
                    ):
                        count += 1

    if count > 0:
        return count * bit
    return bit / 8


def Put(flist, voxel, v0, verts_in, res, g):
    """add voxel key with partial volume value to dict of surface voxels"""
    # v0 is the start coordinates of voxel (verts[0] feed in)
    # res is resolution of sampling points (only works for even values of res!)
    dx, dy, dz = g["dx"], g["dy"], g["dz"]
    Sx, Sy, Sz = dx / res, dy / res, dz / res

    count = 0
    startpt = v0[0] + Sx / 2.0, v0[1] + Sy / 2.0, v0[2] + Sz / 2.0
    for i in range(res):
        for j in range(res):
            for k in range(res):
                v = startpt[0] + i * Sx, startpt[1] + j * Sy, startpt[2] + k * Sz
                if any(
                    f.distance(v[0], v[1], v[2]) <= options.ics_distance_threshold
                    for f in flist
                ):
                    count += 1
    if count > 0:
        return Sx * Sy * Sz * count
    return add_res(flist, voxel, verts_in, 2 * res, g)


def simplevolume(flist, distances, voxel, g):
    """estimate the volume by the fraction of interior points"""
    res = options.ics_partial_volume_resolution
    distance_threshold = options.ics_distance_threshold

    sx, sy, sz = g["dx"] / res, g["dy"] / res, g["dz"] / res

    res1 = res + 1

    # count up the interior points on the boundary
    if res == 1:
        count = sum(1 if d <= distance_threshold else 0 for d in distances)

    else:
        vi, vj, vk = voxel
        startx, starty, startz = (
            g["xlo"] + vi * g["dx"],
            g["ylo"] + vj * g["dy"],
            g["zlo"] + vk * g["dz"],
        )

        count = 0
        for i in range(res1):
            vx = startx + i * sx
            for j in range(res1):
                vy = starty + j * sy
                for k in range(res1):
                    vz = startz + k * sz
                    if any(f.distance(vx, vy, vz) <= distance_threshold for f in flist):
                        count += 1
    volume = count * g["dx"] * g["dy"] * g["dz"] / (res1**3)
    return volume
