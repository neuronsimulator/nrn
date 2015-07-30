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

#ifndef nmodl1_redef_h
#define nmodl1_redef_h

#define v _v
#define area _area
#define thisnode _thisnode
#define GC _GC
#define EC _EC
#define extnode _extnode
#define xain _xain
#define xbout _xbout
#define i _i
#define sec _sec

#undef Memb_list
#undef nodelist
#undef nodeindices
#undef data
#undef pdata
#undef prop
#undef nodecount
#undef type
#undef pval
#undef id
#undef weights
#undef weight_index_

#define NrnThread _NrnThread
#define Memb_list _Memb_list
#define nodelist _nodelist
#define nodeindices _nodeindices
#define data _data
#define pdata _pdata
#define prop _prop
#define nodecount _nodecount
#define type _type
#define pval _pval
#define id _id
#define weights _weights
#define weight_index_ _weight_index

#endif
