from . import graphicsPrimitives as graphics
from .. import options


def find_voxel(x, y, z, g):
    """returns (i,j,k) of voxel containing point x,y,z if the point is within
    the grid, otherwise return the corresponding grid boundary.
    """
    # g is grid boundaries
    i = max(0, int((x - g["xlo"]) // g["dx"]))
    j = max(0, int((y - g["ylo"]) // g["dy"]))
    k = max(0, int((z - g["zlo"]) // g["dz"]))
    return (i, j, k)


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


def get_surr_rows(row, endpoints, g):
    """return list (len=4) of the rows surrounding the current one on all sides
    IF the surrounding row is within the bounds of the grid."""
    (y, z) = row
    surr = []
    if y >= 1:
        surr.append(((y - 1, z), endpoints))
    if z >= 1:
        surr.append(((y, z - 1), endpoints))
    if (g["ylo"] + (y + 1) * g["dy"]) < g["yhi"]:
        surr.append(((y + 1, z), endpoints))
    if (g["zlo"] + (z + 1) * g["dz"]) < g["zhi"]:
        surr.append(((y, z + 1), endpoints))
    return surr


def verts_in(f, voxel, surf, g):
    """return the number of vertices of this voxel that are contained within the surface"""
    verts = get_verts(voxel, g)
    ins = 0
    distlist = []
    for (x, y, z) in verts:
        if (
            g["xlo"] <= x <= g["xhi"]
            and g["ylo"] <= y <= g["yhi"]
            and g["zlo"] <= z <= g["zhi"]
        ):
            dist = f.distance(x, y, z)
        else:
            dist = float("inf")
        distlist.append(dist)
        if dist <= options.ics_distance_threshold:
            ins += 1
    if 1 <= ins <= 7:
        surf[voxel] = distlist
    return ins


def find_endpoints(f, surf, include_ga, row, guesses, g):
    """return the endpoints (L,R) contained in the frustum f; if only one voxel both endpoints will be the same; if none both will be None
    f: frustum object
    surf: surface voxels
    row: current row
    guesses: estimates for endpoints
    g: grid boundaries"""
    # +x or right endpoint
    Rend, Lend = None, None
    check_surf_L, check_surf_R = (None, None), (None, None)
    stop = False
    Ri = guesses[1]
    ogrverts = verts_in(f, (Ri, row[0], row[1]), surf, g)
    if ogrverts == 0:
        going_in = True
    elif 1 <= ogrverts < 8:
        going_in = False
        check_surf_R = (True, Ri)
    else:
        going_in = False
    while (0 <= Ri and (g["xlo"] + (Ri) * g["dx"]) < g["xhi"]) and not stop:
        verts = verts_in(f, (Ri, row[0], row[1]), surf, g)
        if verts == 0:
            if not going_in:
                stop = True
                continue
            else:
                if Ri == guesses[0]:
                    # row is empty between guesses
                    return (None, None)
                Ri -= 1
                continue
        elif verts == 8:
            Rend = Ri
            Ri += 1
            continue
        else:
            Rend = Ri
            if going_in:
                check_surf_R = (True, Ri)
                break
            Ri += 1

    # the -x or left endpoint
    stop = False
    Li = guesses[0]
    oglverts = verts_in(f, (Li, row[0], row[1]), surf, g)
    if oglverts == 0:
        going_in = True
    elif 1 <= oglverts < 8:
        going_in = False
        check_surf_L = (True, Li)
    else:
        going_in = False
    while (0 <= Li and (g["xlo"] + (Li) * g["dx"]) < g["xhi"]) and not stop:
        verts = verts_in(f, (Li, row[0], row[1]), surf, g)
        if verts == 0:
            if not going_in:
                stop = True
                continue
            else:
                # it's not empty or would have already returned
                Li += 1
                continue
        elif verts == 8:
            Lend = Li
            Li -= 1
            continue
        else:
            Lend = Li
            if going_in:
                check_surf_L = (True, Li)
                break
            Li -= 1

    # check for extra surface voxels missed
    if check_surf_R[0] and Lend is not None:
        r = check_surf_R[1]
        while r > Lend:
            verts = verts_in(f, (r, row[0], row[1]), surf, g)
            if verts == 8:
                break
            else:
                r -= 1

    if check_surf_L[0] and Rend is not None:
        l = check_surf_L[1]
        while l < Rend:
            verts = verts_in(f, (l, row[0], row[1]), surf, g)
            if verts == 8:
                break
            else:
                l += 1

    # if keeping non-surface but grid-adjacent voxels:
    if include_ga:
        surf.add((Lend, row[0], row[1]))
        surf.add((Rend, row[0], row[1]))

    return (Lend, Rend)


def voxelize(grid, Object, corners=None, include_ga=False):
    """return a list of all voxels (i,j,k) that contain part of the object
    Other returned elements: set of surface voxels, possibly_missed for error handling"""
    # include_ga is whether to include grid-adjacent voxels in the surface, even if entirely within the surface

    yes_voxels = set()
    checked = set()
    surface = {}

    if corners is not None:
        for i in range(4):
            x0, y0, z0 = corners[i][0], corners[i][1], corners[i][2]
            (i0, j0, k0) = find_voxel(x0, y0, z0, grid)

            # find the contained endpoints and start the set with initial row and initial endpoints
            s = set()
            ends = find_endpoints(Object, surface, include_ga, (j0, k0), (i0, i0), grid)
            if ends[0]:
                break

    else:
        if isinstance(Object, graphics.Sphere):
            x0, y0, z0 = Object.x, Object.y, Object.z
        else:
            x0, y0, z0 = Object._x0, Object._y0, Object._z0

        # find the starting voxel
        (i0, j0, k0) = find_voxel(x0, y0, z0, grid)
        # find the contained endpoints and start the set with initial row and initial endpoints
        s = set()
        ends = find_endpoints(
            Object, surface, include_ga, (j0, k0), (i0 - 1, i0 + 1), grid
        )

    # the given starting voxel is not actually found
    possibly_missed = False
    if not ends[0]:
        possibly_missed = True
        ends = (i0, i0)

    # ------

    for i in range(ends[0], ends[1] + 1):
        yes_voxels.add((i, j0, k0))
    # add that initial row to checked and the set (otherwise inital voxel missed)
    checked.add((j0, k0))
    s.add(((j0, k0), ends))
    while s:
        startr = s.pop()
        newr = get_surr_rows(startr[0], startr[1], grid)
        for r in newr:
            (row, guesses) = r
            if row not in checked:
                (Lend, Rend) = find_endpoints(
                    Object, surface, include_ga, row, guesses, grid
                )
                if Lend is not None:
                    for i in range(Lend, Rend + 1):
                        yes_voxels.add((i, row[0], row[1]))
                    s.add((row, (Lend, Rend)))
                checked.add(row)

    missed = False
    if possibly_missed and (
        len(yes_voxels) == 1
    ):  # no voxels were found, return empty set
        missed = (i0, j0, k0)
        yes_voxels = set()
    return [yes_voxels, surface, missed]
