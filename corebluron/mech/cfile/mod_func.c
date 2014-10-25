/*
Copyright (c) 2014 EPFL-BBP, All rights reserved.

THIS SOFTWARE IS PROVIDED BY THE BLUE BRAIN PROJECT "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE BLUE BRAIN PROJECT
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
extern int nrnmpi_myid;
extern int nrn_nobanner_;
extern int _CaDynamics_E2_reg(void),
  _Ca_HVA_reg(void),
  _Ca_LVAst_reg(void),
  _Ca_reg(void),
  _HDF5reader_reg(void),
  _Ih_reg(void),
  _Im_reg(void),
  _KdShu2007_reg(void),
  _K_Pst_reg(void),
  _K_Tst_reg(void),
  _Nap_Et2_reg(void),
  _NaTa_t_reg(void),
  _NaTs2_t_reg(void),
  _netstim_inhpoisson_reg(void),
  _new_calcium_channels_reg(void),
  _ProbAMPANMDA_EMS_reg(void),
  _ProbGABAAB_EMS_reg(void),
  _SK_E2_reg(void),
  _SKv3_1_reg(void),
 _StochKv_reg(void);

void modl_reg(){
  if (!nrn_nobanner_) if (nrnmpi_myid < 1) {
    fprintf(stderr, " Additional mechanisms from files\n");

    fprintf(stderr," CaDynamics_E2.mod");
    fprintf(stderr," Ca_HVA.mod");
    fprintf(stderr," Ca_LVAst.mod");
    fprintf(stderr," Ca.mod");
    fprintf(stderr," HDF5reader.mod");
    fprintf(stderr," Ih.mod");
    fprintf(stderr," Im.mod");
    fprintf(stderr, "\n");
    fprintf(stderr," KdShu2007.mod");
    fprintf(stderr," K_Pst.mod");
    fprintf(stderr," K_Tst.mod");
    fprintf(stderr," Nap_Et2.mod");
    fprintf(stderr," NaTa_t.mod");
    fprintf(stderr," NaTs2_t.mod");
    fprintf(stderr," netstim_inhpoisson.mod");
    fprintf(stderr, "\n");
    fprintf(stderr," new_calcium_channels.mod");
    fprintf(stderr," ProbAMPANMDA_EMS.mod");
    fprintf(stderr," ProbGABAAB_EMS.mod");
    fprintf(stderr," SK_E2.mod");
    fprintf(stderr," SKv3_1.mod");
    fprintf(stderr," StochKv.mod");
    fprintf(stderr, "\n\n");
  }
  _CaDynamics_E2_reg();
  _Ca_HVA_reg();
  _Ca_LVAst_reg();
  _Ca_reg();
  _HDF5reader_reg();
  _Ih_reg();
  _Im_reg();
  _KdShu2007_reg();
  _K_Pst_reg();
  _K_Tst_reg();
  _Nap_Et2_reg();
  _NaTa_t_reg();
  _NaTs2_t_reg();
  _netstim_inhpoisson_reg();
  _new_calcium_channels_reg();
  _ProbAMPANMDA_EMS_reg();
  _ProbGABAAB_EMS_reg();
  _SK_E2_reg();
  _SKv3_1_reg();
  _StochKv_reg();
}
