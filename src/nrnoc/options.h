/*
  Options are either very significant to the usage and/or affect
  the size of the main data structures and thus require much recompilation,
  including recompilation of all models.
*/

/* switching from Ra being a global variable to it being a section variable
opens up the possibility of a great deal of confusion and inadvertant wrong
results. To help avoid this we print a warning message whenever the value
in one section is set but no others. But only the first time through treeset.
*/
#define RA_WARNING 0

#define I_MEMBRANE 1 /* compute i_cap and i_membrane on fadvance */

#define EXTRACELLULAR 2 /* default number of extracellular layers */

#define DIAMLIST 1 /* section contains diameter info */
#define EXTRAEQN                             \
    0 /* ionic concentrations calculated via \
       * jacobian along with v */
#if DIAMLIST
#define NTS_SPINE                                      \
    1 /* A negative diameter in pt3dadd() tags that    \
       * diamlist location as having a spine.          \
       * diam3d() still returns the postive diameter   \
       * spined3d() returns 1 or 0 signifying presence \
       * of spine. setSpineArea() tells how much       \
       * area/spine to add to the segment. */
#endif

#define KEEP_NSEG_PARM 1 /* Use old segment parameters to define */
/* the new segment information */

#if !defined(CACHEVEC)
#define CACHEVEC 1 /* define to 0 doubles in nodes instead of vectors*/
#endif

#define MULTICORE 1 /* not optional */
