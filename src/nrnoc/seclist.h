#pragma once

struct Section;

void SectionList_reg(void);
void lvappendsec_and_ref(void* sl, Section* sec);
Section* nrn_secarg(int iarg);
double seclist_size(void* sl);
void forall_sectionlist();
void hoc_ifseclist();
