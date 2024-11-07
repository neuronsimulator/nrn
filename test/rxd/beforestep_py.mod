: Python callback from BEFORE STEP

NEURON {
  POINT_PROCESS beforestep_callback
  POINTER ptr     
}

ASSIGNED {
  ptr     
}

INITIAL {
}

VERBATIM
extern int (*nrnpy_hoccommand_exec)(Object*);
extern Object** hoc_objgetarg(int);
extern int ifarg(int);
extern void hoc_obj_ref(Object*);
extern void hoc_obj_unref(Object*);
ENDVERBATIM

BEFORE STEP {
  :printf("beforestep_callback t=%g\n", t)
VERBATIM
{
  Object* cb = (Object*)(_p_ptr);
  if (cb) {
    (*nrnpy_hoccommand_exec)(cb);
  }
}
ENDVERBATIM
}

PROCEDURE set_callback() {
VERBATIM
  Object** pcb = (Object**)(&(_p_ptr));
  if (*pcb) {
    hoc_obj_unref(*pcb);
    *pcb = (Object*)0;
  }
  if (ifarg(1)) {
    *pcb = *(hoc_objgetarg(1));
    hoc_obj_ref(*pcb);
  }
ENDVERBATIM
}

