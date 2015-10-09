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

#ifndef nrnoc_ml_h
#define nrnoc_ml_h

#include "coreneuron/nrnconf.h"

typedef union ThreadDatum {
	double val;
	int i;
	double* pval;
	void* _pvoid;
}ThreadDatum;

typedef struct NetReceiveBuffer_t {
	int* _pnt_index;
	int* _weight_index;
	double* _nrb_t;
	int _cnt;
	int _size; /* capacity */
	int _pnt_offset;
}NetReceiveBuffer_t;

typedef struct Memb_list {
#if CACHEVEC != 0
	/* nodeindices contains all nodes this extension is responsible for,
	 * ordered according to the matrix. This allows to access the matrix
	 * directly via the nrn_actual_* arrays instead of accessing it in the
	 * order of insertion and via the node-structure, making it more
	 * cache-efficient */
	int *nodeindices;
#endif /* CACHEVEC */
	double* data;
	Datum* pdata;
	ThreadDatum* _thread; /* thread specific data (when static is no good) */
	NetReceiveBuffer_t* _net_receive_buffer;
	int nodecount;
} Memb_list;

#endif
