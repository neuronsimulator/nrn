#pragma once
struct NrnThread;
void update_actual_rhs_based_on_sp13_rhs(NrnThread* nt);
void update_sp13_rhs_based_on_actual_rhs(NrnThread* nt);
