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

    explicit data_reader(const char *filename,bool reorder=false);
    
    /** Preserving chkpnt state, move to a new file. */
    void open(const char *filename, bool reorder);

    /** Is the file not open */
    bool fail() const {return F.fail();}

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
    inline T* parse_array(T* p,size_t count,parse_action flag){
        if (count>0 && flag!=seek) nrn_assert(p!=0);
  
        read_checkpoint_assert();
        switch (flag) {
        case seek:
            F.seekg(count*sizeof(T),std::ios_base::cur);
            break;
        case read:
            F.read((char *)p,count*sizeof(T));
            if (reorder_on_read) endian::swap_endian_range(p,p+count);
            break;
        }
  
        nrn_assert(!F.fail());
        return p;
    }

    // convenience interfaces:

    /** Read and optionally allocate an integer array of fixed length. */
    template <typename T>
    inline T* read_array(T* p,size_t count) { return parse_array(p,count,read); }

    /** Allocate and read an integer array of fixed length. */
    template <typename T>
    inline T* read_array(size_t count) { return parse_array(new T[count],count,read); }
      
    /** Close currently open file. */
    void close();
};


#endif // ifndef nrn_datareader_h
