#pragma once

extern bool splitfor();
extern void splitfor_current();
extern void splitfor_cur(int);
extern void splitfor_solve();

extern int brkpnt_exists;
extern List* currents;
extern List* solvq;
extern List* useion;

// used by the nrn_cur fragment for splitfor_cur
extern List* conductance_;
extern List* get_ion_variables(int);
extern List* set_ion_variables(int);
extern List* state_discon_list_;
extern int point_process;
extern int electrode_current;
extern int artificial_cell;
extern Symbol* cvode_nrn_cur_solve_;
