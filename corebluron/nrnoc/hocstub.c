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

#include <stdio.h>
#include <stdlib.h>

double* hoc_pgetarg(int iarg) { (void)iarg; printf("ERROR in hocstub.c! Function hoc_pgetarg() should never be called! Exiting..."); abort(); }
double* getarg(int iarg) { (void)iarg; printf("ERROR in hocstub.c! Function getarg() should never be called! Exiting..."); abort(); }
char* gargstr(int iarg) { (void)iarg; printf("ERROR in hocstub.c! Function gargstr() should never be called! Exiting..."); abort(); }
int hoc_is_str_arg(int iarg) { (void)iarg; printf("ERROR in hocstub.c! Function hoc_is_str_arg() should never be called! Exiting..."); abort(); }
int ifarg(int iarg) { (void)iarg; printf("ERROR in hocstub.c! Function ifarg() should never be called! Exiting..."); abort(); }
double chkarg(int iarg, double low, double high) { (void)iarg; (void)low; (void)high; printf("ERROR in hocstub.c! Function chkarg() should never be called! Exiting..."); abort(); }
void* vector_arg(int i) { (void) i; printf("ERROR in hocstub.c! Function vector_arg() should never be called! Exiting..."); abort(); }
