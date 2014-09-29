#include "corebluron/utils/endianness.h"
#include "corebluron/utils/swap_endian.h"
#include "corebluron/nrniv/nrn_datareader.h"
#include "corebluron/nrniv/nrn_assert.h"


data_reader::data_reader(const char *filename,enum endian::endianness file_endianness) {
    this->open(filename,file_endianness);
    checkpoint(0);
}

void data_reader::open(const char *filename,enum endian::endianness file_endianness) {
    reorder_on_read=(file_endianness!=endian::native_endian);
    if (reorder_on_read)
        nrn_assert(file_endianness!=endian::mixed_endian &&
                endian::native_endian!=endian::mixed_endian);

    close();
    F.open(filename);
    nrn_assert(!F.fail());
}

static const int max_line_length=100;

int data_reader::read_int() {
    char line_buf[max_line_length];

    F.getline(line_buf,sizeof(line_buf));
    nrn_assert(!F.fail());

    int i;
    int n_scan=sscanf(line_buf,"%d",&i);
    nrn_assert(n_scan==1);
    
    return i;
}

void data_reader::read_checkpoint_assert() {
    char line_buf[max_line_length];

    F.getline(line_buf,sizeof(line_buf));
    nrn_assert(!F.fail());

    int i;
    int n_scan=sscanf(line_buf,"chkpnt %d\n",&i);
    nrn_assert(n_scan==1);
    nrn_assert(i==chkpnt);
    ++chkpnt;
}


template <typename T>
T *data_reader::parse_array(T *p,size_t count,parse_action flag) {
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

/** Explicit instantiation of parse_array<in> */
template int *data_reader::parse_array<int>(int *,size_t,parse_action);

/** Explicit instantiation of parse_array<double> */
template double *data_reader::parse_array<double>(double *,size_t,parse_action);

void data_reader::close() {
    F.close();
}

