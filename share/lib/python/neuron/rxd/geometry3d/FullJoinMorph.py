import numpy
import time
import math

from .ctng import constructive_neuronal_geometry
from .graphicsPrimitives import Sphere, Cone, Cylinder, SkewCone, Plane, Union, Intersection, SphereCone
from .GeneralizedVoxelization import voxelize
from .simplevolume_helper import simplevolume
from .surface_a import surface_area
import warnings
from neuron import h

def find_parent_seg(join, sdict, objects):
    
    if not join:
        return None
    elif join[0] not in objects:
        pseg = sdict[(join[0]._x0, join[0]._y0, join[0]._z0, join[0]._x1, join[0]._y1, join[0]._z1)]
        # better be all in same cell; so just set root once
        h.distance(0, h.SectionRef(sec=pseg.sec).root(0))
        closest = h.distance(pseg)

    # any other possible instance?

    for item in join:
        if item not in objects:
            s = sdict[(item._x0, item._y0, item._z0, item._x1, item._y1, item._z1)]
            d = h.distance(s)
            if d < closest:
                pseg = s
                closest = d

    return pseg
 

def all_in(dist):
    for i in dist:
        if i > 0:
            return False 
    return True

def sort_spheres_last(item):
    return 1 if isinstance(item, Sphere) else 0

def fullmorph(source, dx, soma_step=100):
    start = time.time()

    """Input: object source; arguments to pass to ctng
       Output: all voxels with SA and volume associated, categorized by segment"""
    
    morphology = constructive_neuronal_geometry(source, dx, soma_step)
    join_objects, cones, segment_dict, join_groups, object_pts, soma_objects = morphology

    # grid setup
    xs, ys, zs, diams, arcs = [],[],[],[],[]
    for sec in source:
    	xs += [sec.x3d(i) for i in range(sec.n3d())]
    	ys += [sec.y3d(i) for i in range(sec.n3d())]
    	zs += [sec.z3d(i) for i in range(sec.n3d())]
    	diams += [sec.diam3d(i) for i in range(sec.n3d())]
    	arcs += [sec.arc3d(i + 1) - sec.arc3d(i) for i in range(sec.n3d() - 1)]

    # warning on minimum size of dx
    check = min(min(diams)/math.sqrt(3), min(arcs)/math.sqrt(3))
    if (dx > check):
        warnings.warn("Resolution may be too low. To guarantee accurate voxelization, use a dx <= {}.".format(check))
    
    dy = dz = dx   # ever going to change this?

    margin = max(diams) + dx
    grid = {'xlo':min(xs)-margin, 'xhi':max(xs)+margin, 'ylo':min(ys)-margin, 'yhi':max(ys)+margin, 'zlo':min(zs)-margin, 'zhi':max(zs)+margin, 'dx':dx, 'dy':dy, 'dz':dz}

    ##########################################################
    final_seg_dict = {}

    # soma: modified when ctng properly assigns soma obj to segments
    if soma_objects:
        for item in soma_objects:
            seg = segment_dict[item]
            if seg in final_seg_dict.keys():
                final_seg_dict[seg].append(item)
            else:
                final_seg_dict[seg] = [item]
    
    # assign join objects
    for jg in join_groups:
        seg = find_parent_seg(jg, segment_dict, join_objects)
        for item in jg:
            #if isinstance(item, Sphere): continue # TODO: don't do this... just a test
            if (not(isinstance(item, Cone) or isinstance(item, Cylinder))) or (item in join_objects):
                if seg in final_seg_dict.keys():
                    final_seg_dict[seg].append(item)
                else: 
                    final_seg_dict[seg] = [item]


    # complete final segment dictionary
    for cone in cones:
        seg = segment_dict[(cone._x0, cone._y0, cone._z0, cone._x1, cone._y1, cone._z1)]
        if seg in final_seg_dict.keys():
            final_seg_dict[seg].append(cone)
        else:
            final_seg_dict[seg] = [cone]

    # voxelize all the objects and assign voxels
 	# output dictionaries of internal and surface voxels
 	# assign voxels to segments
    missed = 0
    missed_voxels = set()
    total_surface_voxels = {}
    final_intern_voxels = {}			# final output of internal voxels

    for do_spheres in [False, True]:
        for seg in final_seg_dict:
            print()
            print(seg,':')
            distance_root = h.SectionRef(sec=seg.sec).root(0)
            for item in [my_item for my_item in final_seg_dict[seg] if isinstance(my_item, Sphere) == do_spheres]:
                print(item)
                if item in object_pts:
                    [yesvox, surface, miss] = voxelize(grid, item, object_pts[item])
                else:
                    [yesvox, surface, miss] = voxelize(grid, item)
                if miss:
                    missed += 1
                    missed_voxels.add(miss)

                # must take only the internal voxels for that item (set diff)
                yesvox = yesvox - surface.keys()
                for i in yesvox:  
                    if i in final_intern_voxels.keys():
                        if h.distance(distance_root, seg) < h.distance(distance_root, final_intern_voxels[i][1]) and not isinstance(item, Sphere):
                            final_intern_voxels[i][1] = seg
                    else:                 
                        final_intern_voxels[i] = [dx**3, seg]

                for i in surface.keys():
                    if i in total_surface_voxels.keys():				
                        total_surface_voxels[i][0].append(item)
                        # update the distances list to the minimum at each vertex
                        total_surface_voxels[i][1] = [min(total_surface_voxels[i][1][j], surface[i][j]) for j in range(8)]
                        if h.distance(distance_root, seg) < h.distance(distance_root, total_surface_voxels[i][2]) and not isinstance(item, Sphere):
                            total_surface_voxels[i][2] = seg
                    else:
                        total_surface_voxels[i] = [[item], surface[i], seg]

    # take internal voxels out of surface voxels
    for vox in final_intern_voxels.keys():
    	if vox in total_surface_voxels.keys():
    		del total_surface_voxels[vox]

    # calculate volume and SA for surface voxels, taking into account multiple item crossovers
    final_surface_voxels = {}
    for vox in total_surface_voxels.keys():
    	# modified version of SV1 functions verts_in that returns volume and distances
        [itemlist, distances, seg] = total_surface_voxels[vox]
        # check if the surface voxel is actually internal after updating all vertex distances:
        if all_in(distances):
            final_intern_voxels[vox] = [dx**3, seg]
            # no need to delete from surface voxels, it is never added to final surface voxels
        else:
            V = simplevolume(itemlist, distances, vox, grid)
            A = surface_area(distances[0], distances[1], distances[2], distances[3], distances[4], distances[5], distances[6], distances[7], 0,dx,0,dy,0,dz)
            final_surface_voxels[vox] = [V, A, seg]

    # make sure to keep track of poss_missed for each cone; total should be 0
    """if missed > 0:
        print("{} objects have inaccurate voxelization, probably due to resolution errors.".format(missed))
        print("Missed voxels total: ", len(missed_voxels))
    """
    return final_intern_voxels, final_surface_voxels, grid























