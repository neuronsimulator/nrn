/*
    This file contains code adapted from p3d.py in
    http://code.google.com/p/pythonisosurfaces/source/checkout
    which was released under the new BSD license.
    accessed 31 July 2012
*/

#include <math.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cassert>
//#include <nrnpython.h>

int geometry3d_find_triangles(double value0,
                              double value1,
                              double value2,
                              double value3,
                              double value4,
                              double value5,
                              double value6,
                              double value7,
                              double x0,
                              double x1,
                              double y0,
                              double y1,
                              double z0,
                              double z1,
                              double* out,
                              int offset);

double geometry3d_llgramarea(double* p0, double* p1, double* p2);
double geometry3d_sum_area_of_triangles(double* tri_vec, int len);

const int edgeTable[] = {
    0x0,   0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c, 0x80c, 0x905, 0xa0f, 0xb06, 0xc0a,
    0xd03, 0xe09, 0xf00, 0x190, 0x99,  0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c, 0x99c, 0x895,
    0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90, 0x230, 0x339, 0x33,  0x13a, 0x636, 0x73f, 0x435,
    0x53c, 0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30, 0x3a0, 0x2a9, 0x1a3, 0xaa,
    0x7a6, 0x6af, 0x5a5, 0x4ac, 0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0, 0x460,
    0x569, 0x663, 0x76a, 0x66,  0x16f, 0x265, 0x36c, 0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963,
    0xa69, 0xb60, 0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff,  0x3f5, 0x2fc, 0xdfc, 0xcf5, 0xfff,
    0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0, 0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55,  0x15c,
    0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950, 0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6,
    0x2cf, 0x1c5, 0xcc,  0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0, 0x8c0, 0x9c9,
    0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc, 0xcc,  0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9,
    0x7c0, 0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c, 0x15c, 0x55,  0x35f, 0x256,
    0x55a, 0x453, 0x759, 0x650, 0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc, 0x2fc,
    0x3f5, 0xff,  0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0, 0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f,
    0xd65, 0xc6c, 0x36c, 0x265, 0x16f, 0x66,  0x76a, 0x663, 0x569, 0x460, 0xca0, 0xda9, 0xea3,
    0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac, 0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa,  0x1a3, 0x2a9, 0x3a0,
    0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c, 0x53c, 0x435, 0x73f, 0x636, 0x13a,
    0x33,  0x339, 0x230, 0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c, 0x69c, 0x795,
    0x49f, 0x596, 0x29a, 0x393, 0x99,  0x190, 0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905,
    0x80c, 0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0};

/* CTNG:tritable */
const int triTable[256][16] = {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
                               {3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
                               {3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
                               {3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
                               {9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
                               {1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
                               {9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
                               {2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
                               {8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
                               {9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
                               {4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
                               {3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
                               {1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
                               {4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
                               {4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
                               {9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
                               {1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
                               {5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
                               {2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
                               {9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
                               {0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
                               {2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
                               {10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
                               {4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
                               {5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
                               {5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
                               {9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
                               {0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
                               {1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
                               {10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
                               {8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
                               {2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
                               {7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
                               {9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
                               {2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
                               {11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
                               {9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
                               {5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
                               {11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
                               {11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
                               {1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
                               {9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
                               {5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
                               {2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
                               {0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
                               {5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
                               {6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
                               {0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
                               {3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
                               {6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
                               {5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
                               {1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
                               {10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
                               {6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
                               {1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
                               {8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
                               {7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
                               {3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
                               {5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
                               {0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
                               {9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
                               {8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
                               {5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
                               {0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
                               {6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
                               {10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
                               {10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
                               {8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
                               {1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
                               {3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
                               {0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
                               {10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
                               {0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
                               {3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
                               {6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
                               {9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
                               {8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
                               {3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
                               {6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
                               {0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
                               {10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
                               {10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
                               {1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
                               {2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
                               {7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
                               {7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
                               {2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
                               {1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
                               {11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
                               {8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
                               {0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
                               {7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
                               {10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
                               {2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
                               {6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
                               {7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
                               {2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
                               {1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
                               {10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
                               {10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
                               {0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
                               {7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
                               {6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
                               {8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
                               {9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
                               {6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
                               {1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
                               {4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
                               {10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
                               {8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
                               {0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
                               {1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
                               {8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
                               {10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
                               {4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
                               {10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
                               {5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
                               {11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
                               {9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
                               {6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
                               {7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
                               {3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
                               {7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
                               {9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
                               {3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
                               {6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
                               {9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
                               {1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
                               {4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
                               {7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
                               {6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
                               {3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
                               {0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
                               {6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
                               {1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
                               {0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
                               {11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
                               {6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
                               {5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
                               {9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
                               {1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
                               {1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
                               {10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
                               {0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
                               {5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
                               {10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
                               {11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
                               {0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
                               {9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
                               {7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
                               {2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
                               {8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
                               {9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
                               {9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
                               {1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
                               {9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
                               {9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
                               {5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
                               {0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
                               {10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
                               {2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
                               {0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
                               {0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
                               {9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
                               {5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
                               {3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
                               {5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
                               {8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
                               {0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
                               {9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
                               {0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
                               {1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
                               {3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
                               {4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
                               {9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
                               {11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
                               {11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
                               {2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
                               {9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
                               {3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
                               {1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
                               {4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
                               {4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
                               {0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
                               {3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
                               {3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
                               {0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
                               {9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
                               {1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
                               {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}};


/* CTNG:interpedge */
void geometry3d_vi(double* p1, double* p2, double v1, double v2, double* out) {
    if (fabs(v1) < 0.000000000001) {
        out[0] = p1[0];
        out[1] = p1[1];
        out[2] = p1[2];
        return;
    }
    if (fabs(v2) < 0.000000000001) {
        out[0] = p2[0];
        out[1] = p2[1];
        out[2] = p2[2];
        return;
    }
    double delta_v = v1 - v2;
    if (fabs(delta_v) < 0.0000000001) {
        out[0] = p1[0];
        out[1] = p1[1];
        out[2] = p1[2];
        return;
    }
    double mu = v1 / delta_v;
    if (std::isnan(mu)) {
        printf("geometry3d_vi error. delta_v = %g, v1 = %g, v2 = %g\n", delta_v, v1, v2);
    }
    out[0] = p1[0] + mu * (p2[0] - p1[0]);
    out[1] = p1[1] + mu * (p2[1] - p1[1]);
    out[2] = p1[2] + mu * (p2[2] - p1[2]);
}

int geometry3d_find_triangles(double value0,
                              double value1,
                              double value2,
                              double value3,
                              double value4,
                              double value5,
                              double value6,
                              double value7,
                              double x0,
                              double x1,
                              double y0,
                              double y1,
                              double z0,
                              double z1,
                              double* out,
                              int offset) {
    out = out + offset;
    double position[8][3] = {{x0, y0, z0},
                             {x1, y0, z0},
                             {x1, y1, z0},
                             {x0, y1, z0},
                             {x0, y0, z1},
                             {x1, y0, z1},
                             {x1, y1, z1},
                             {x0, y1, z1}};

    /* CTNG:domarch */
    int cubeIndex = 0;
    if (value0 < 0)
        cubeIndex |= 1;
    if (value1 < 0)
        cubeIndex |= 2;
    if (value2 < 0)
        cubeIndex |= 4;
    if (value3 < 0)
        cubeIndex |= 8;
    if (value4 < 0)
        cubeIndex |= 16;
    if (value5 < 0)
        cubeIndex |= 32;
    if (value6 < 0)
        cubeIndex |= 64;
    if (value7 < 0)
        cubeIndex |= 128;

    int et = edgeTable[cubeIndex];

    if (et == 0)
        return 0;

    double vertexList[12][3];
    if (et & 1)
        geometry3d_vi(position[0], position[1], value0, value1, vertexList[0]);
    if (et & 2)
        geometry3d_vi(position[1], position[2], value1, value2, vertexList[1]);
    if (et & 4)
        geometry3d_vi(position[2], position[3], value2, value3, vertexList[2]);
    if (et & 8)
        geometry3d_vi(position[3], position[0], value3, value0, vertexList[3]);
    if (et & 16)
        geometry3d_vi(position[4], position[5], value4, value5, vertexList[4]);
    if (et & 32)
        geometry3d_vi(position[5], position[6], value5, value6, vertexList[5]);
    if (et & 64)
        geometry3d_vi(position[6], position[7], value6, value7, vertexList[6]);
    if (et & 128)
        geometry3d_vi(position[7], position[4], value7, value4, vertexList[7]);
    if (et & 256)
        geometry3d_vi(position[0], position[4], value0, value4, vertexList[8]);
    if (et & 512)
        geometry3d_vi(position[1], position[5], value1, value5, vertexList[9]);
    if (et & 1024)
        geometry3d_vi(position[2], position[6], value2, value6, vertexList[10]);
    if (et & 2048)
        geometry3d_vi(position[3], position[7], value3, value7, vertexList[11]);

    int const* const tt = triTable[cubeIndex];
    int i, j, k, count;
    for (i = 0, count = 0; i < 16; i += 3, count++) {
        if (tt[i] == -1)
            break;
        for (k = 0; k < 3; k++) {
            for (j = 0; j < 3; j++)
                out[j] = vertexList[tt[i + k]][j];
            out += 3;
        }
    }
    return count;
}


double geometry3d_llgramarea(double* p0, double* p1, double* p2) {
    /* setup the vectors */
    double a[] = {p0[0] - p1[0], p0[1] - p1[1], p0[2] - p1[2]};
    double b[] = {p0[0] - p2[0], p0[1] - p2[1], p0[2] - p2[2]};

    /* take the cross-product */
    double cpx = a[1] * b[2] - a[2] * b[1];
    double cpy = a[2] * b[0] - a[0] * b[2];
    double cpz = a[0] * b[1] - a[1] * b[0];
    return std::sqrt(cpx * cpx + cpy * cpy + cpz * cpz);
}


double geometry3d_sum_area_of_triangles(double* tri_vec, int len) {
    double sum = 0;
    for (int i = 0; i < len; i += 9) {
        sum += geometry3d_llgramarea(tri_vec + i, tri_vec + i + 3, tri_vec + i + 6);
    }
    return sum / 2.;
}


/*****************************************************************************
 * this contains cone, and cylinder code translated and modified
 * from Barbier and Galin 2004's example code
 * see /u/ramcd/spatial/experiments/one_time_tests/2012-06-28/cone.cpp
 * on 7 june 2012, the original code was available online at
 *   http://jgt.akpeters.com/papers/BarbierGalin04/Cone-Sphere.zip
 ****************************************************************************/


class geometry3d_Cylinder {
  public:
    geometry3d_Cylinder(double x0, double y0, double z0, double x1, double y1, double z1, double r);
    //~geometry3d_Cylinder();
    double signed_distance(double px, double py, double pz);

  private:
    // double x0, y0, z0, x1, y1, z1, r;
    double r, rr, axisx, axisy, axisz, cx, cy, cz, h;
};

geometry3d_Cylinder::geometry3d_Cylinder(double x0,
                                         double y0,
                                         double z0,
                                         double x1,
                                         double y1,
                                         double z1,
                                         double r)
    : r(r)
    , cx((x0 + x1) / 2.)
    , cy((y0 + y1) / 2.)
    , cz((z0 + z1) / 2.)
    , rr(r * r) {
    axisx = x1 - x0;
    axisy = y1 - y0;
    axisz = z1 - z0;
    double axislength = std::sqrt(axisx * axisx + axisy * axisy + axisz * axisz);
    axisx /= axislength;
    axisy /= axislength;
    axisz /= axislength;
    h = axislength / 2.;
}

double geometry3d_Cylinder::signed_distance(double px, double py, double pz) {
    double const nx{px - cx};
    double const ny{py - cy};
    double const nz{pz - cz};
    double y{std::abs(axisx * nx + axisy * ny + axisz * nz)};
    double yy{y * y};
    double const xx{nx * nx + ny * ny + nz * nz - yy};
    if (y < h) {
        return std::max(-std::abs(h - y), std::sqrt(xx) - r);
    } else {
        y -= h;
        yy = y * y;
        if (xx < rr) {
            return std::abs(y);
        } else {
            double const x{std::sqrt(xx) - r};
            return std::sqrt(yy + x * x);
        }
    }
}

void* geometry3d_new_Cylinder(double x0,
                              double y0,
                              double z0,
                              double x1,
                              double y1,
                              double z1,
                              double r) {
    return new geometry3d_Cylinder(x0, y0, z0, x1, y1, z1, r);
}
void geometry3d_delete_Cylinder(void* ptr) {
    delete (geometry3d_Cylinder*) ptr;
}
// TODO: add validation for ptr
double geometry3d_Cylinder_signed_distance(void* ptr, double px, double py, double pz) {
    return ((geometry3d_Cylinder*) ptr)->signed_distance(px, py, pz);
}


class geometry3d_Cone {
  public:
    geometry3d_Cone(double x0,
                    double y0,
                    double z0,
                    double r0,
                    double x1,
                    double y1,
                    double z1,
                    double r1);
    double signed_distance(double px, double py, double pz);

  private:
    double axisx, axisy, axisz, h, rra, rrb, conelength;
    double side1, side2, x0, y0, z0, r0, axislength;
};

geometry3d_Cone::geometry3d_Cone(double x0,
                                 double y0,
                                 double z0,
                                 double r0,
                                 double x1,
                                 double y1,
                                 double z1,
                                 double r1)
    : rra(r0 * r0)
    , rrb(r1 * r1)
    , x0(x0)
    , y0(y0)
    , z0(z0)
    , r0(r0) {
    // TODO: these are preconditions; the python assures them, but could/should
    //       take care of that here
    assert(r1 <= r0);
    assert(r1 >= 0);
    axisx = x1 - x0;
    axisy = y1 - y0;
    axisz = z1 - z0;
    axislength = std::sqrt(axisx * axisx + axisy * axisy + axisz * axisz);
    axisx /= axislength;
    axisy /= axislength;
    axisz /= axislength;
    h = axislength / 2.;
    rra = r0 * r0;
    rrb = r1 * r1;
    conelength = std::sqrt((r1 - r0) * (r1 - r0) + axislength * axislength);
    side1 = (r1 - r0) / conelength;
    side2 = axislength / conelength;
}

double geometry3d_Cone::signed_distance(double px, double py, double pz) {
    double nx, ny, nz, y, yy, xx, x, rx, ry;
    nx = px - x0;
    ny = py - y0;
    nz = pz - z0;
    y = axisx * nx + axisy * ny + axisz * nz;
    yy = y * y;
    xx = nx * nx + ny * ny + nz * nz - yy;
    // in principle, xx >= 0, but roundoff errors may cause trouble
    if (xx < 0)
        xx = 0;
    if (y < 0) {
        if (xx < rra)
            return -y;
        x = std::sqrt(xx) - r0;
        return std::sqrt(x * x + yy);
    } else if (xx < rrb) {
        return y - axislength;
    } else {
        x = std::sqrt(xx) - r0;
        if (y < 0) {
            if (x < 0)
                return y;
            return std::sqrt(x * x + yy);
        } else {
            ry = x * side1 + y * side2;
            if (ry < 0)
                return std::sqrt(x * x + yy);
            rx = x * side2 - y * side1;
            if (ry > conelength) {
                ry -= conelength;
                return std::sqrt(rx * rx + ry * ry);
            } else {
                return rx;
            }
        }
    }
}


void* geometry3d_new_Cone(double x0,
                          double y0,
                          double z0,
                          double r0,
                          double x1,
                          double y1,
                          double z1,
                          double r1) {
    return new geometry3d_Cone(x0, y0, z0, r0, x1, y1, z1, r1);
}
void geometry3d_delete_Cone(void* ptr) {
    delete (geometry3d_Cone*) ptr;
}
// TODO: add validation for ptr
double geometry3d_Cone_signed_distance(void* ptr, double px, double py, double pz) {
    return ((geometry3d_Cone*) ptr)->signed_distance(px, py, pz);
}


class geometry3d_Sphere {
  public:
    geometry3d_Sphere(double x, double y, double z, double r);
    double signed_distance(double px, double py, double pz);

  private:
    double x, y, z, r;
};

geometry3d_Sphere::geometry3d_Sphere(double x, double y, double z, double r)
    : x(x)
    , y(y)
    , z(z)
    , r(r) {}

double geometry3d_Sphere::signed_distance(double px, double py, double pz) {
    return std::sqrt(std::pow(x - px, 2) + std::pow(y - py, 2) + std::pow(z - pz, 2)) - r;
}

void* geometry3d_new_Sphere(double x, double y, double z, double r) {
    return new geometry3d_Sphere(x, y, z, r);
}
void geometry3d_delete_Sphere(void* ptr) {
    delete (geometry3d_Sphere*) ptr;
}
// TODO: add validation for ptr
double geometry3d_Sphere_signed_distance(void* ptr, double px, double py, double pz) {
    return ((geometry3d_Sphere*) ptr)->signed_distance(px, py, pz);
}

class geometry3d_Plane {
  public:
    geometry3d_Plane(double x, double y, double z, double nx, double ny, double nz);
    double signed_distance(double px, double py, double pz);

  private:
    double nx, ny, nz;
    double d, mul;
};

geometry3d_Plane::geometry3d_Plane(double x, double y, double z, double nx, double ny, double nz)
    : nx(nx)
    , ny(ny)
    , nz(nz)
    , d(-(nx * x + ny * y + nz * z))
    , mul(1. / std::sqrt(nx * nx + ny * ny + nz * nz)) {}

double geometry3d_Plane::signed_distance(double px, double py, double pz) {
    return (nx * px + ny * py + nz * pz + d) * mul;
}

void* geometry3d_new_Plane(double x, double y, double z, double nx, double ny, double nz) {
    return new geometry3d_Plane(x, y, z, nx, ny, nz);
}
void geometry3d_delete_Plane(void* ptr) {
    delete (geometry3d_Plane*) ptr;
}
// TODO: add validation for ptr
double geometry3d_Plane_signed_distance(void* ptr, double px, double py, double pz) {
    return ((geometry3d_Plane*) ptr)->signed_distance(px, py, pz);
}

/*
    PyObject* nrnpy_pyCallObject(PyObject*, PyObject*);

    void print_numbers(PyObject *p) {
        for (Py_ssize_t i = 0; i< PyList_Size(p); i++) {
            PyObject* obj = PyList_GetItem(p, i);
            printf("%g ", PyFloat_AsDouble(obj));
        }
        printf("\n");
    }

    // TODO: it would be nice to remove the python dependence, because that
    //       limits us to mostly single threaded due to the global interpreter
    //       lock
    int geometry3d_process_cell(int i, int j, int k, PyObject* objects, double* xs, double* ys,
double* zs, double* tridata, int start) { double x, y, z, x1, y1, z1, xx, yy, zz; x = xs[i]; y =
ys[j]; z = zs[k]; x1 = xs[i + 1]; y1 = ys[j + 1]; z1 = zs[k + 1]; double value[8], current_value;
        PyObject* result;
        PyObject* obj;
        printf("inside process_cell\n");

        // march around the cube
        for (int m = 0; m < 8; m++) {
            // NOTE: only describing changes from previous case
            switch(m) {
                case 0: xx = x; yy = y; zz = z; break;
                case 1: xx = x1; break;
                case 2: yy = y1; break;
                case 3: xx = x; break;
                case 4: yy = y; zz = z1; break;
                case 5: xx = x1; break;
                case 6: yy = y1; break;
                case 7: xx = x; break;
            }
            printf("phase 0, len(objects) = %ld\n", PyList_Size(objects));
            obj = PyList_GetItem(objects, 0);
            printf("phase 0a, obj = %x\n", (void*) obj);
            result = PyEval_CallMethod(obj, "distance", "ddd", xx, yy, zz);
            //Py_DECREF(obj);
            printf("phase 1\n");
            current_value = PyFloat_AsDouble(result);
            //Py_DECREF(result);
            for (Py_ssize_t n = 1; n < PyList_Size(objects); n++) {
                printf("phase 2, n = %ld\n", n);
                obj = PyList_GetItem(objects, n);
                result = PyEval_CallMethod(obj, "distance", "ddd", xx, yy, zz);
                Py_DECREF(obj);
                current_value = min(current_value, PyFloat_AsDouble(result));
                //Py_DECREF(result);
            }
            value[m] = current_value;
        }
        printf("finishing up; start = %d\n", start);
        return start + 9 * geometry3d_find_triangles(value[0], value[1], value[2], value[3],
value[4], value[5], value[6], value[7], x, x1, y, y1, z, z1, tridata, start);
    }


    int geometry3d_test_call_function(PyObject* obj) {
        printf("inside\n");
        if (obj == NULL) printf("obj is NULL\n");
        Py_INCREF(obj);
        PyEval_CallObject(obj, obj);
        return 0;
    }

    int geometry3d_test_call_function3(int (*func) (void)) {
        return func();
    }

    int geometry3d_test_call_method(PyObject* list, PyObject* obj) {
        PyEval_CallMethod(list, "insert", "O", obj);
        return 0;
    }

    // this works, but is slower than the python version
    int geometry3d_contains_surface(int i, int j, int k, double (*objdist)(double, double, double),
double* xs, double* ys, double* zs, double dx, double r_inner, double r_outer) { bool has_neg =
false, has_pos = false; double xbar = xs[i] + dx / 2; double ybar = ys[j] + dx / 2; double zbar =
zs[k] + dx / 2;

        double d = fabs(objdist(xbar, ybar, zbar));
        //if (i == 586 && j == 2169 && k == 83) {printf("at magic point, d=%g\n", d);}
        //printf("sphere test: d = %g, r_inner = %g, r_outer = %g\n", d, r_inner, r_outer);
        if (d <= r_inner) return 1;
        if (d >= r_outer) return 0;
//        // spheres alone are indeterminant; check corners
//        for (int di = 0; di < 2; di++) {
//            for (int dj = 0; dj < 2; dj++) {
//                for (int dk = 0; dk < 2; dk++) {
//                    d = objdist(xs[i + di], ys[j + dj], zs[k + dk]);
//                    //printf("d = %g\n", d);
//                    if (d <= 0) has_neg = true;
//                    if (d >= 0) has_pos = true;
//                    if (has_neg && has_pos) return 1;
//                }
//            }
//        }

        // spheres alone are indeterminant; check corners
        for (double* x = xs + i; x < xs + i + 2; x++) {
            for (double* y = ys + j; y < ys + j + 2; y++) {
                for (double* z = zs + k; z < zs + k + 2; z++) {
                    d = objdist(*x, *y, *z);
                    if (d <= 0) has_neg = true;
                    if (d >= 0) has_pos = true;
                    if (has_neg && has_pos) return 1;
                }
            }
        }

        return 0;
    }

}
*/
