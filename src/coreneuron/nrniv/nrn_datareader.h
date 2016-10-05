/*
Copyright (c) 2016, Blue Brain Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef nrn_datareader_h
#define nrn_datareader_h

#include <fstream>
#include "coreneuron/utils/endianness.h"
#include "coreneuron/utils/swap_endian.h"
#include "coreneuron/nrniv/nrn_assert.h"

/** Encapsulate low-level reading of coreneuron input data files.
 *
 * Error handling is simple: abort()!
 *
 * Reader will abort() if native integer size is not 4 bytes,
 * or if need a convoluted byte reordering between native
 * and file endianness. Complicated endianness configurations and
 * the possibility of non-IEEE floating point representations
 * are happily ignored.
 *
 * All automatic allocations performed by read_int_array()
 * and read_dbl_array() methods use new [].
 */

class data_reader {
    std::ifstream F;       //!< File stream associated with reader.
    bool reorder_on_read;  //!< True if we need to reorder for native endiannes.
    int chkpnt;            //!< Current checkpoint number state.

    /** Read a checkpoint line, bump our chkpnt counter, and assert equality.
     *
     * Checkpoint information is represented by a sequence "checkpt %d\n"
     * where %d is a scanf-compatible representation of the checkpoint
     * integer.
     */
    void read_checkpoint_assert();

    // private copy constructor, assignment: data_reader is not copyable.
    data_reader(const data_reader&);
    data_reader& operator=(const data_reader&);

  public:
    data_reader() : reorder_on_read(false), chkpnt(0) {
    }

    explicit data_reader(const char* filename, bool reorder = false);

    /** Preserving chkpnt state, move to a new file. */
    void open(const char* filename, bool reorder);

    /** Is the file not open */
    bool fail() const {
        return F.fail();
    }

    /** Query chkpnt state. */
    int checkpoint() const {
        return chkpnt;
    }

    /** Explicitly override chkpnt state. */
    void checkpoint(int c) {
        chkpnt = c;
    }

    /** Parse a single integer entry.
     *
     * Single integer entries are represented by their standard
     * (C locale) text representation, followed by a newline.
     * Extraneous characters following the integer but preceding
     * the newline are ignore.
     */
    int read_int();

    /** Parse a neuron mapping count entries
     *
     * Reads neuron mapping info which is represented by
     * gid, #segments, #somas, #axons, #dendrites, #apicals, #total compartments
     */
    void read_mapping_count(int* gid,
                            int* seg,
                            int* soma,
                            int* axon,
                            int* dend,
                            int* apical,
                            int* compartment);

    /** Parse a neuron section segment mapping
     *
     * Read count no of mappings for section to segment
     */
    template <typename T>
    void read_mapping_info(T* mapinfo, int count) {
        const int max_line_length = 1000;
        char line_buf[max_line_length];

        for (int i = 0; i < count; i++) {
            F.getline(line_buf, sizeof(line_buf));
            nrn_assert(!F.fail());
            int sec, seg, n_scan;
            n_scan = sscanf(line_buf, "%d %d", &sec, &seg);
            nrn_assert(n_scan == 2);
            mapinfo->add_segment(sec, seg);
        }
    }

    /** Defined flag values for parse_array() */
    typedef enum parse_action { read, seek } parse_action;

    /** Generic parse function for an array of fixed length.
      *
      * \tparam T the array element type: may be \c int or \c double.
      * \param p pointer to the target in memory for reading into.
      * \param count number of items of type \a T to parse.
      * \param action whether to validate and skip (\c seek) or
      *    copy array into memory (\c read).
      * \return the supplied pointer value.
      *
      * Error if \a count is non-zero, \a flag is \c read, and
      * the supplied pointer \p is null.
      *
      * Arrays are represented by a checkpoint line followed by
      * the array items in increasing index order, in the native binary
      * representation of the writing process.
      */
    template <typename T>
    inline T* parse_array(T* p, size_t count, parse_action flag) {
        if (count > 0 && flag != seek)
            nrn_assert(p != 0);

        read_checkpoint_assert();
        switch (flag) {
            case seek:
                F.seekg(count * sizeof(T), std::ios_base::cur);
                break;
            case read:
                F.read((char*)p, count * sizeof(T));
                if (reorder_on_read)
                    endian::swap_endian_range(p, p + count);
                break;
        }

        nrn_assert(!F.fail());
        return p;
    }

    // convenience interfaces:

    /** Read and optionally allocate an integer array of fixed length. */
    template <typename T>
    inline T* read_array(T* p, size_t count) {
        return parse_array(p, count, read);
    }

    /** Allocate and read an integer array of fixed length. */
    template <typename T>
    inline T* read_array(size_t count) {
        return parse_array(new T[count], count, read);
    }

    /** Close currently open file. */
    void close();
};

#endif  // ifndef nrn_datareader_h
