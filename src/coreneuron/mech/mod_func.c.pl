#!/ usr / bin / perl

#Copyright(c) 2014 EPFL - BBP, All rights reserved.
#
#THIS SOFTWARE IS PROVIDED BY THE BLUE BRAIN PROJECT "AS IS"
#AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
#THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
#PURPOSE ARE DISCLAIMED.IN NO EVENT SHALL THE BLUE BRAIN PROJECT
#BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
#BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
#WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING NEGLIGENCE
#OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
#IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#Construct the modl_reg() function from a provided list
#of modules.

#Usage : mod_func.c.pl[MECH1.mod MECH2.mod...]

@mods = @ARGV;
s/\.mod$// foreach @mods;

@mods=sort @mods;

print << "__eof";
#include <stdio.h>
extern int nrnmpi_myid;
extern int nrn_nobanner_;
extern int @{[join ",\n  ", map{"_${_}_reg(void)"} @mods]};

void modl_reg() {
    if (!nrn_nobanner_)
        if (nrnmpi_myid < 1) {
            fprintf(stderr, " Additional mechanisms from files\\n");

            @{[join "\n", map{"    fprintf(stderr,\" $_.mod\");"} @mods] } fprintf(stderr,
                                                                                   "\\n\\n");
        }

    @{[join "\n", map{" _${_}_reg();"} @mods] }
}
__eof
