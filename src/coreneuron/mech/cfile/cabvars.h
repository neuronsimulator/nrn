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


extern  void capac_reg_(void), _passive_reg(void),
#if EXTRACELLULAR
	extracell_reg_(void),
#endif
	_stim_reg(void),
	_hh_reg(void),
        _netstim_reg(void),
	_expsyn_reg(void);

static void (*mechanism[])(void) = { /* type will start at 3 */
	capac_reg_,
	_passive_reg,
#if EXTRACELLULAR
	/* extracellular requires special handling and must be type 5 */
	extracell_reg_,
#endif
	_stim_reg,
	_hh_reg,
	_expsyn_reg,
        _netstim_reg,
	0
};

#if 0
static char *morph_mech[] = { /* this is type 2 */
	"0", "morphology", "diam", 0,0,0,
};

extern void cab_alloc(void);
extern void morph_alloc(void);
#endif

#if 0
 first two memb_func
	NULL_CUR, NULL_ALLOC, NULL_STATE, NULL_INITIALIZE, (Pfri)0, 0,	/*Unused*/
	NULL_CUR, cab_alloc, NULL_STATE, NULL_INITIALIZE, (Pfri)0, 0,	/*CABLESECTION*/
#endif
