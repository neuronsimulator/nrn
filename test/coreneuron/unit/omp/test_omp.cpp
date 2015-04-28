#define BOOST_TEST_MODULE omp 
#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>
#include <omp.h>
#include "nrnomp/nrnomp.h"

#define NUMTHREAD 4

#if defined(_OPENMP)
extern void omp_set_num_threads(int num);
#endif

BOOST_AUTO_TEST_CASE(omp_standardusecase) 
{
#if defined(_OPENMP)
    omp_set_num_threads(NUMTHREAD);
    BOOST_CHECK(nrnomp_get_numthreads()==NUMTHREAD);
#endif
}



    
