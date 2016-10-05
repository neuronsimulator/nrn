# Copyright (c) 2016, Blue Brain Project
# All rights reserved.

# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.


find_program(SLURM_SBATCH_COMMAND sbatch DOC "Path to the SLURM sbatch executable")
find_program(SLURM_SRUN_COMMAND srun DOC "Path to the SLURM srun executable")
find_program(SLURM_SACCTMGR_COMMAND sacctmgr DOC "Path to the SLURM sacctmgr executable")
mark_as_advanced(SLURM_SRUN_COMMAND SLURM_SBATCH_COMMAND SLURM_SACCTMGR_COMMAND)

if(SLURM_SRUN_COMMAND AND SLURM_SBATCH_COMMAND)
  set(SLURM_FOUND true)
  if(NOT SLURM_FIND_QUIETLY)
    message (STATUS "Found SLURM. SLURM_SBATCH_COMMAND: ${SLURM_SBATCH_COMMAND}, SLURM_SRUN_COMMAND: ${SLURM_SRUN_COMMAND}, SLURM_SACCTMGR_COMMAND: ${SLURM_SACCTMGR_COMMAND}")
  endif()
else()
  set(SLURM_FOUND false )
  if(SLURM_FIND_REQUIRED)
    message(FATAL_ERROR "Could not find SLURM")
  elseif(NOT SLURM_FIND_QUIETLY)
    message(STATUS "Could not find SLURM")
  endif()
endif()

