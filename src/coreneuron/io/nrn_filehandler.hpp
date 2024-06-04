/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/

#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <sys/stat.h>

#include "coreneuron/utils/nrn_assert.h"
#include "coreneuron/io/nrnsection_mapping.hpp"

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

    // FileHandler is not copyable.
    FileHandler(const FileHandler&) = delete;
    FileHandler& operator=(const FileHandler&) = delete;

  public:
    FileHandler()
        : chkpnt(0)
        , stored_chkpnt(0) {}

    explicit FileHandler(const std::string& filename);

    /** Preserving chkpnt state, move to a new file. */
    void open(const std::string& filename, std::ios::openmode mode = std::ios::in);

    /** Is the file not open */
    bool fail() const {
        return F.fail();
    }

    static bool file_exist(const std::string& filename);

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
    int read_mapping_info(T* mapinfo, NrnThreadMappingInfo* ntmapping, CellMapping* cmap) {
        int nsec, nseg, n_scan;
        size_t total_lfp_factors;
        int num_electrodes;
        char line_buf[max_line_length], name[max_line_length];

        F.getline(line_buf, sizeof(line_buf));
        n_scan = sscanf(
            line_buf, "%s %d %d %zd %d", name, &nsec, &nseg, &total_lfp_factors, &num_electrodes);

        nrn_assert(n_scan == 5);

        mapinfo->name = std::string(name);

        if (nseg) {
            auto sec = read_vector<int>(nseg);
            auto seg = read_vector<int>(nseg);

            std::vector<double> lfp_factors;
            if (total_lfp_factors > 0) {
                lfp_factors = read_vector<double>(total_lfp_factors);
            }

            int factor_offset = 0;
            for (int i = 0; i < nseg; i++) {
                mapinfo->add_segment(sec[i], seg[i]);
                ntmapping->add_segment_id(seg[i]);
                int factor_offset = i * num_electrodes;
                if (total_lfp_factors > 0) {
                    // Abort if the factors contains a NaN
                    nrn_assert(count_if(lfp_factors.begin(), lfp_factors.end(), [](double d) {
                                   return std::isnan(d);
                               }) == 0);
                    std::vector<double> segment_factors(lfp_factors.begin() + factor_offset,
                                                        lfp_factors.begin() + factor_offset +
                                                            num_electrodes);
                    cmap->add_segment_lfp_factor(seg[i], segment_factors);
                }
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
            F.read((char*) p, count * sizeof(T));
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
        F.write((const char*) p, nb_elements * (sizeof(T)));
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
        F.write((const char*) temp_cpy, nb_elements * sizeof(T) * nb_lines);
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
