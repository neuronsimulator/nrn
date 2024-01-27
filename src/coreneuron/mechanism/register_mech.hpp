/*
# =============================================================================
# Copyright (c) 2016 - 2021 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================
*/
#pragma once

namespace coreneuron {
void add_nrn_artcell(int type, int qi);
void set_pnt_receive(int type,
                     pnt_receive_t pnt_receive,
                     pnt_receive_t pnt_receive_init,
                     short size);
extern void initnrn(void);
extern void hoc_reg_bbcore_read(int type, bbcore_read_t f);
extern void hoc_reg_bbcore_write(int type, bbcore_write_t f);
extern void _nrn_thread_table_reg(
    int i,
    void (*f)(int, int, double*, Datum*, ThreadDatum*, NrnThread*, Memb_list*, int));
extern void alloc_mech(int);

}  // namespace coreneuron
