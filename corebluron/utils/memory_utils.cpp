/**
 * @file memory_utils.cpp
 * @date 25th Oct 2014
 *
 * @brief Provides functionality to report current memory usage
 * of the simulator using interface provided by malloc.h
 *
 * Memory utilisation report is based on the use of mallinfo
 * interface defined in malloc.h. For 64 bit platform, this
 * is not portable and hence it will be replaced with new
 * glibc implementation of malloc_info.
 *
 * @see http://man7.org/linux/man-pages/man3/malloc_info.3.html
 */

#include <stdio.h>
#include "memory_utils.h"
#include "corebluron/nrnmpi/nrnmpi.h"

#ifdef HAVE_MEMORY_H
#include <spi/include/kernel/memory.h>
#elif defined HAVE_MALLOC_H
#include <malloc.h>
#endif


double nrn_mallinfo( void )
{
  double mbs = -1.0; // -ve mem usage if mallinfo is not supported


// On BG-Q, Use kernel/memory.h to get heap statistics
#ifdef HAVE_MEMORY_H
  uint64_t heap = 0;
  Kernel_GetMemorySize(KERNEL_MEMSIZE_HEAP, &heap);
  mbs = heap / (1024.0 * 1024.0);
// if malloc.h available, collect information from mallinfo  
#elif defined HAVE_MALLOC_H
  struct mallinfo m;
  
  m =  mallinfo();
  mbs = ( m.hblkhd + m.uordblks ) /  (1024.0 * 1024.0);
#endif
  
  return mbs;
}


void report_mem_usage( const char *message, bool all_ranks )
{
  double cur_mem, mem_max, mem_min, mem_avg; //min, max, avg memory 
  
  // current memory usage on this rank
  cur_mem = nrn_mallinfo();
  
  /* @todo: avoid three all reduce class */
  mem_avg = nrnmpi_dbl_allreduce( cur_mem, 1 ) / nrnmpi_numprocs;
  mem_max = nrnmpi_dbl_allreduce( cur_mem, 2 );
  mem_min = nrnmpi_dbl_allreduce( cur_mem, 3 );
  
  // all ranks prints information if all_ranks is true
  if ( all_ranks ) {
    printf( " Memory (MBs) (Rank : %2d) : %30s : Cur %.4lf, Max %.4lf, Min %.4lf, Avg %.4lf \n", \
            nrnmpi_myid, message, cur_mem, mem_max, mem_min, mem_avg );
  }
  else if ( nrnmpi_myid == 0 ) {
  
    printf( " Memory (MBs) : %25s : Max %.4lf, Min %.4lf, Avg %.4lf \n", \
            message, mem_max, mem_min, mem_avg );
  }
}


