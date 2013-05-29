#include <math.h>

double llgramarea(double* p0, double* p1, double* p2) {
    /* setup the vectors */
    double a[] = {p0[0] - p1[0], p0[1] - p1[1], p0[2] - p1[2]};
    double b[] = {p0[0] - p2[0], p0[1] - p2[1], p0[2] - p2[2]};
    
    /* take the cross-product */
    double cpx = a[1] * b[2] - a[2] * b[1];
    double cpy = a[2] * b[0] - a[0] * b[2];
    double cpz = a[0] * b[1] - a[1] * b[0];
    return sqrt(cpx * cpx + cpy * cpy + cpz * cpz);    
}


double llpipedfromoriginvolume(double* p0, double* p1, double* p2) {
    /* take the cross-product */
    double cpx = p1[1] * p2[2] - p1[2] * p2[1];
    double cpy = p1[2] * p2[0] - p1[0] * p2[2];
    double cpz = p1[0] * p2[1] - p1[1] * p2[0];

    /* vol = p0.dot(numpy.cross(p1, p2)) / 6. */
    return p0[0] * cpx + p0[1] * cpy + p0[2] * cpz;
}


