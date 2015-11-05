#include "coreneuron/nrniv/nrn_datareader.h"


data_reader::data_reader(const char *filename, bool reorder) {
    this->open(filename,reorder);
    checkpoint(0);
}

void data_reader::open(const char *filename, bool reorder) {
    reorder_on_read=reorder;

    close();
    F.open(filename);
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
    if (n_scan != 1) {
      fprintf(stderr, "no chkpnt line for %d\n", chkpnt);
    }
    nrn_assert(n_scan==1);
    if (i != chkpnt) {
      fprintf(stderr, "file chkpnt %d != expected %d\n", i, chkpnt);
    }
    nrn_assert(i==chkpnt);
    ++chkpnt;
}


void data_reader::close() {
    F.close();
}

