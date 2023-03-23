#pragma once
#include "hocstr.h"
#include "nrnmpiuse.h"

#include <stdio.h>
#include <stdlib.h>

using NrnFILEWrap = FILE;
#define nrn_fw_wrap(f)       f
#define nrn_fw_delete(fw)    /**/
#define nrn_fw_eq(fw, ff)    (fw == ff)
#define nrn_fw_fclose        fclose
#define nrn_fw_set_stdin()   stdin
#define nrn_fw_fopen         fopen
#define nrn_fw_fseek         fseek
#define nrn_fw_getc(fw)      getc(fw)
#define nrn_fw_ungetc(c, fw) ungetc(c, fw)
#define nrn_fw_fscanf        fscanf

extern char* fgets_unlimited(HocStr* s, NrnFILEWrap* f);
extern NrnFILEWrap* hoc_fin;
