#ifndef register_mech_h
#define register_mech_h

namespace coreneuron {
extern void hoc_reg_bbcore_read(int type, bbcore_read_t f);
extern void hoc_reg_bbcore_write(int type, bbcore_write_t f);
extern void _nrn_thread_table_reg(
    int i,
    void (*f)(int, int, double*, Datum*, ThreadDatum*, NrnThread*, int));

}  // namespace coreneuron

#endif
