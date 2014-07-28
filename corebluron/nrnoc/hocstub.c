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

/* hoc functions that might be called from mod files should abort if
   actually used.
*/

#include <assert.h>

double* hoc_pgetarg(int iarg) { (void)iarg; assert(0); }
double* getarg(int iarg) { (void)iarg; assert(0); }
char* gargstr(int iarg) { (void)iarg; assert(0); }
int hoc_is_str_arg(int iarg) { (void)iarg; assert(0); }
int ifarg(int iarg) { (void)iarg; assert(0); }
double chkarg(int iarg, double low, double high) {
	(void)iarg; (void)low; (void)high;
	 assert(0);
}
void* vector_arg(int i) { (void) i; assert(0); }
