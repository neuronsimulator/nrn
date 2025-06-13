#pragma once

/* interface to Random123 */
/* http://www.thesalmons.org/john/random123/papers/random123sc11.pdf */

/*
The 4x32 generators utilize a uint32x4 counter and uint32x4 key to transform
into an almost cryptographic quality uint32x4 random result.
There are many possibilites for balancing the sharing of the internal
state instances while reserving a uint32 counter for the stream sequence
and reserving other portions of the counter vector for stream identifiers
and global index used by all streams.

We currently provide a single instance by default in which the policy is
to use the 0th counter uint32 as the stream sequence, words 2, 3 and 4 as the
stream identifier, and word 0 of the key as the global index. Unused words
are constant uint32 0.

It is also possible to use Random123 directly without reference to this
interface. See Random123-1.02/docs/html/index.html
of the full distribution available from
http://www.deshawresearch.com/resources_random123.html
*/

#include <cstdint>


struct nrnran123_State;

struct nrnran123_array4x32 {
    std::uint32_t v[4];
};

/* global index. eg. run number */
/* all generator instances share this global index */
extern void nrnran123_set_globalindex(std::uint32_t gix);
extern std::uint32_t nrnran123_get_globalindex();

/** Construct a new Random123 stream based on the philox4x32 generator.
 *  
 *  @param id1 stream ID
 *  @param id2 optional defaults to 0
 *  @param id3 optional defaults to 0
 *  @return an nrnran123_State object representing this stream
 */
extern nrnran123_State* nrnran123_newstream(std::uint32_t id1,
                                            std::uint32_t id2 = 0,
                                            std::uint32_t id3 = 0);

/** @deprecated use nrnran123_newstream instead **/
extern nrnran123_State* nrnran123_newstream3(std::uint32_t id1,
                                             std::uint32_t id2,
                                             std::uint32_t id3);

/** Construct a new Random123 stream based on the philox4x32 generator.
 *
 *  @note This overload constructs each stream instance as an independent stream.
 *  Independence is derived by using id1=1, id2=nrnmpi_myid,
 *  id3 = ++internal_static_uint32_initialized_to_0
 */
extern nrnran123_State* nrnran123_newstream();

/** Destroys the given Random123 stream. */
extern void nrnran123_deletestream(nrnran123_State* s);

/** Get sequence number and selector from an nrnran123_State object */
extern void nrnran123_getseq(nrnran123_State* s, std::uint32_t* seq, char* which);

/** Set a Random123 sequence for a sequnece ID and which selector.
 *
 * @param s an Random123 state object
 * @param seq the sequence ID for which to initialize the random number sequence
 * @param which the selector (0 <= which < 4) of the sequence
 */
extern void nrnran123_setseq(nrnran123_State* s, std::uint32_t seq, char which);

/** Set a Random123 sequence for a sequnece ID and which selector.
 *
 * This overload encodes the sequence ID and which in one double. This is done specifically to be
 * able to expose the Random123 API in HOC, which only supports real numbers.
 *
 * @param s an Random123 state object
 * @param seq4which encodes both seq and which as seq*4+which
 */
extern void nrnran123_setseq(nrnran123_State* s, double seq4which);  // seq*4+which);

/** Get stream IDs from Random123 State object */
extern void nrnran123_getids(nrnran123_State* s, std::uint32_t* id1, std::uint32_t* id2);


/** Get stream IDs from Random123 State object */
extern void nrnran123_getids(nrnran123_State* s,
                              std::uint32_t* id1,
                              std::uint32_t* id2,
                              std::uint32_t* id3);

/** @brief. Deprecated, use nrnran123_getids **/
extern void nrnran123_getids3(nrnran123_State*,
                              std::uint32_t* id1,
                              std::uint32_t* id2,
                              std::uint32_t* id3);

extern void nrnran123_setids(nrnran123_State*, std::uint32_t id1, std::uint32_t id2, std::uint32_t id3);

// Get a random uint32_t in [0, 2^32-1]
extern std::uint32_t nrnran123_ipick(nrnran123_State*);
// Get a random double on [0, 1]
// nrnran123_dblpick minimum value is 2.3283064e-10 and max value is 1-min
extern double nrnran123_dblpick(nrnran123_State*);

/* nrnran123_negexp min value is 2.3283064e-10, max is 22.18071 if mean is 1.0 */
extern double nrnran123_negexp(nrnran123_State*); // mean = 1.0
extern double nrnran123_negexp(nrnran123_State*, double mean);
extern double nrnran123_normal(nrnran123_State*); // mean = 0.0, std = 1.0
extern double nrnran123_normal(nrnran123_State*, double mean, double std);

extern double nrnran123_uniform(nrnran123_State*); // same as dblpick
extern double nrnran123_uniform(nrnran123_State*, double min, double max);

/* more fundamental (stateless) (though the global index is still used) */
extern nrnran123_array4x32 nrnran123_iran(std::uint32_t seq, std::uint32_t id1, std::uint32_t id2 = 0, std::uint32_t id3 = 0);

/** @brief. Deprecated, use nrnran123_iran **/
extern nrnran123_array4x32 nrnran123_iran3(std::uint32_t seq,
                                           std::uint32_t id1,
                                           std::uint32_t id2,
                                           std::uint32_t id3);
