#ifndef nrn_datareader_h
#define nrn_datareader_h

#include <fstream>
#include "corebluron/utils/endianness.h"

/** Encapsulate low-level reading of corebluron input data files.
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
    int chkpnt;             //!< Current checkpoint number state.

    /** Read a checkpoint line, bump our chkpnt counter, and assert equality.
     *
     * Checkpoint information is represented by a sequence "checkpt %d\n"
     * where %d is a scanf-compatible representation of the checkpoint
     * integer.
     */
    void read_checkpoint_assert();

    // private copy constructor, assignment: data_reader is not copyable.
    data_reader(const data_reader &);
    data_reader &operator=(const data_reader &);

public:
    data_reader(): reorder_on_read(false),chkpnt(0) {}

    explicit data_reader(const char *filename,enum endian::endianness=endian::native_endian);
    
    /** Preserving chkpnt state, move to a new file. */
    void open(const char *filename,enum endian::endianness=endian::native_endian);

    /** Query chkpnt state. */
    int checkpoint() const { return chkpnt; }

    /** Explicitly override chkpnt state. */
    void checkpoint(int c) { chkpnt=c; }

    /** Parse a single integer entry.
     *
     * Single integer entries are represented by their standard
     * (C locale) text representation, followed by a newline.
     * Extraneous characters following the integer but preceding
     * the newline are ignore.
     */
    int read_int();

    /** Defined flag values for parse_array() */
    typedef enum parse_action { read,seek } parse_action;
    
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
    T *parse_array(T *p,size_t count,parse_action flag);

    // convenience interfaces:

    /** Read and optionally allocate an integer array of fixed length. */
    int *read_int_array(int *p,size_t count) { return parse_array(p,count,read); }

    /** Allocate and read an integer array of fixed length. */
    int *read_int_array(size_t count) { return parse_array(new int[count],count,read); }
      
    /** Parse a double array of fixed length. */
    double *read_dbl_array(double *p,size_t count) { return parse_array(p,count,read); }

    /** Allocate (new []) and parse a double array of fixed length. */
    double *read_dbl_array(size_t count) { return parse_array(new double[count],count,read); }

    /** Close currently open file. */
    void close();
};


#endif // ifndef nrn_datareader_h
