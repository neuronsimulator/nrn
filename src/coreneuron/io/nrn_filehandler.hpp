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

#ifndef nrn_filehandler_h
#define nrn_filehandler_h

#include <iostream>
#include <fstream>
#include <vector>
#include <sys/stat.h>

#include "coreneuron/utils/nrn_assert.h"

namespace coreneuron {
/** Encapsulate low-level reading of coreneuron input data files.
 *
 * Error handling is simple: abort()!
 *
 * Reader will abort() if native integer size is not 4 bytes.
 *
 * All automatic allocations performed by read_int_array()
 * and read_dbl_array() methods use new [].
 */

// @todo: remove this static buffer
const int max_line_length = 1024;

class FileHandler {
    std::fstream F;                        //!< File stream associated with reader.
    std::ios_base::openmode current_mode;  //!< File open mode (not stored in fstream)
    int chkpnt;                            //!< Current checkpoint number state.
    int stored_chkpnt;                     //!< last "remembered" checkpoint number state.
    /** Read a checkpoint line, bump our chkpnt counter, and assert equality.
     *
     * Checkpoint information is represented by a sequence "checkpt %d\n"
     * where %d is a scanf-compatible representation of the checkpoint
     * integer.
     */
    void read_checkpoint_assert();

    // private copy constructor, assignment: FileHandler is not copyable.
    FileHandler(const FileHandler&);
    FileHandler& operator=(const FileHandler&);

  public:
    FileHandler() : chkpnt(0), stored_chkpnt(0) {
    }

    explicit FileHandler(const std::string& filename);

    /** Preserving chkpnt state, move to a new file. */
    void open(const std::string& filename, std::ios::openmode mode = std::ios::in);

    /** Is the file not open */
    bool fail() const {
        return F.fail();
    }

    bool file_exist(const std::string& filename) const;

    /** nothing more to read */
    bool eof();

    /** Query chkpnt state. */
    int checkpoint() const {
        return chkpnt;
    }

    /** Explicitly override chkpnt state. */
    void checkpoint(int c) {
        chkpnt = c;
    }

    /** Record current chkpnt state. */
    void record_checkpoint() {
        stored_chkpnt = chkpnt;
    }

    /** Restored last recorded chkpnt state. */
    void restore_checkpoint() {
        chkpnt = stored_chkpnt;
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
     * gid, #sections, #segments, #section lists
     */
    void read_mapping_count(int* gid, int* nsec, int* nseg, int* nseclist);

    /** Reads number of cells in parsing file */
    void read_mapping_cell_count(int* count);

    /** Parse a neuron section segment mapping
     *
     * Read count no of mappings for section to segment
     */
    template <typename T>
    int read_mapping_info(T* mapinfo) {
        int nsec, nseg, n_scan;
        char line_buf[max_line_length], name[max_line_length];

        F.getline(line_buf, sizeof(line_buf));
        n_scan = sscanf(line_buf, "%s %d %d", name, &nsec, &nseg);

        nrn_assert(n_scan == 3);

        mapinfo->name = std::string(name);

        if (nseg) {
            std::vector<int> sec, seg;
            sec.reserve(nseg);
            seg.reserve(nseg);

            read_array<int>(&sec[0], nseg);
            read_array<int>(&seg[0], nseg);

            for (int i = 0; i < nseg; i++) {
                mapinfo->add_segment(sec[i], seg[i]);
            }
        }
        return nseg;
    }

    /** Defined flag values for parse_array() */
    enum parse_action { read, seek };

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
                break;
        }

        nrn_assert(!F.fail());
        return p;
    }

    // convenience interfaces:

    /** Read an integer array of fixed length. */
    template <typename T>
    inline T* read_array(T* p, size_t count) {
        return parse_array(p, count, read);
    }

    /** Allocate and read an integer array of fixed length. */
    template <typename T>
    inline T* read_array(size_t count) {
        return parse_array(new T[count], count, read);
    }

    template <typename T>
    inline std::vector<T> read_vector(size_t count) {
        std::vector<T> vec(count);
        parse_array(vec.data(), count, read);
        return vec;
    }

    /** Close currently open file. */
    void close();

    /** Write an 1D array **/
    template <typename T>
    void write_array(T* p, size_t nb_elements) {
        nrn_assert(F.is_open());
        nrn_assert(current_mode & std::ios::out);
        write_checkpoint();
        F.write((const char*)p, nb_elements * (sizeof(T)));
        nrn_assert(!F.fail());
    }

    /** Write a padded array. nb_elements is number of elements to write per line,
     * line_width is full size of a line in nb elements**/
    template <typename T>
    void write_array(T* p,
                     size_t nb_elements,
                     size_t line_width,
                     size_t nb_lines,
                     bool to_transpose = false) {
        nrn_assert(F.is_open());
        nrn_assert(current_mode & std::ios::out);
        write_checkpoint();
        T* temp_cpy = new T[nb_elements * nb_lines];

        if (to_transpose) {
            for (size_t i = 0; i < nb_lines; i++) {
                for (size_t j = 0; j < nb_elements; j++) {
                    temp_cpy[i + j * nb_lines] = p[i * line_width + j];
                }
            }
        } else {
            memcpy(temp_cpy, p, nb_elements * sizeof(T) * nb_lines);
        }
        // AoS never use padding, SoA is translated above, so one write
        // operation is enought in both cases
        F.write((const char*)temp_cpy, nb_elements * sizeof(T) * nb_lines);
        nrn_assert(!F.fail());
        delete[] temp_cpy;
    }

    template <typename T>
    FileHandler& operator<<(const T& scalar) {
        nrn_assert(F.is_open());
        nrn_assert(current_mode & std::ios::out);
        F << scalar;
        nrn_assert(!F.fail());
        return *this;
    }

  private:
    /* write_checkpoint is callable only for our internal uses, making it accesible to user, makes
     * file format unpredictable */
    void write_checkpoint() {
        F << "chkpnt " << chkpnt++ << "\n";
    }
};
}  // namespace coreneuron
#endif  // ifndef nrn_filehandler_h
