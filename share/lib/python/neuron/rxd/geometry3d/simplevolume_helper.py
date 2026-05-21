from .. import options


def simplevolume(flist: list, distances: list, voxel: tuple, g: dict) -> float:
    """estimate the volume by the fraction of interior points"""
    res = options.ics_partial_volume_resolution
    distance_threshold = options.ics_distance_threshold
    
    dx, dy, dz = g["dx"], g["dy"], g["dz"]
    sx, sy, sz = dx / res, dy / res, dz / res

    res1 = res + 1

    # count up the interior points on the boundary
    if res == 1:
        count = sum(1 if d <= distance_threshold else 0 for d in distances)

    else:
        vi, vj, vk = voxel
        startx, starty, startz = (
            g["xlo"] + vi * dx,
            g["ylo"] + vj * dy,
            g["zlo"] + vk * dz,
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
    volume = count * dx * dy * dz / (res1**3)
    return volume
