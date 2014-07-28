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

#ifndef usenrnmpi_h
#define usenrnmpi_h

/* define to 1 if you want MPI specific features activated */
#define NRNMPI 1

/* define to 1 if you want parallel distributed cells (and gap junctions) */
#define PARANEURON 1

/* define to 1 if you want mpi dynamically loaded instead of linked normally */
#undef NRNMPI_DYNAMICLOAD

/* define to 1 if you want the MUSIC - MUlti SImulation Coordinator */
#undef NRN_MUSIC

/* define to the dll path if you want to load automatically */
#undef DLL_DEFAULT_FNAME

/* define if needed */
#undef ALWAYS_CALL_MPI_INIT

/* Number of times to retry a failed open */
#undef FILE_OPEN_RETRY

/* define if IBM BlueGene L, P or Q (activates BGLCheckPoint functionality) */
#undef BLUEGENE

/* define if IBM BlueGene/P */
#undef BLUEGENEP

/* define if IBM BlueGene/Q */
#undef BLUEGENEQ

/* define BlueGene with checkpointing */
#undef BLUEGENE_CHECKPOINT

/* Define bits for BGPDMA & 1 (ISend) & 2 (DMA spike transfer) & 4 (DMA Record Replay */
#undef BGPDMA

/* Define to 1 for possibility of rank 0 xopen/ropen a file and broadcast everywhere */
#undef USE_NRNFILEWRAP

#endif
