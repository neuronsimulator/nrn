#include <nrnmpiuse.h>
#if defined(BGPDMA) && BGPDMA > 1
/* now compiled directly instead of included in bgpdma.cpp because it
   for an unknown reason now causes a bug in the functionality of pc.cell .
*/

/* from curl -k -O https://wiki.alcf.anl.gov/images/8/8d/Meminfo.c */
/* compile with -I/bgsys/drivers/ppcfloor/arch/include */

#undef STR

#include <spi/kernel_interface.h>
#include <common/bgp_personality.h>
#include <common/bgp_personality_inlines.h>
#include <malloc.h>

/* gives total memory size per MPI process */
void
getMemSize(long long *mem)
{
  long long total;
  int node_config;
  _BGP_Personality_t personality;

  Kernel_GetPersonality(&personality, sizeof(personality));
  total = BGP_Personality_DDRSizeMB(&personality);

  node_config  = BGP_Personality_processConfig(&personality);
  if (node_config == _BGP_PERS_PROCESSCONFIG_VNM) total /= 4;
  else if (node_config == _BGP_PERS_PROCESSCONFIG_2x2) total /= 2;
  total *= 1024*1024;

  *mem = total;
}

/* gives total memory used */
void
getUsedMem(long long *mem)
{
  long long alloc;
  struct mallinfo m;

  m = mallinfo();
  alloc = m.hblkhd + m.uordblks;

  *mem = alloc;
}

/* gives available memory */
void
getFreeMem(long long *mem)
{
  long long total, alloc;

  getMemSize(&total);
  getUsedMem(&alloc);

  *mem = total - alloc;
}

#endif /* BGPDMA > 1 */
