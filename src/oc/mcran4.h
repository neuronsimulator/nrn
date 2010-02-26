void mcell_ran4_init();
double mcell_ran4();
unsigned int mcell_iran4();
double mcell_ran4_64();
unsigned int mcell_iran4_64();

/*
Michael Hines added the prefix mcell to the global names.
These functions were obtained from Tom Bartol <bartol@salk.edu>
who uses them in his mcell program.
He comments:
For MCell, Ran4 has the distinct advantage of generating
streams of random bits not just random numbers.  This means that you can
break the 32 bits of a single returned random number into several smaller
chunks without fear of correlations between the chunks.  Ran4 is not the
fastest generator in the universe but it's pretty fast (16 million
floating point random numbers per second on my 1GHz Intel PIII and 20
million integer random numbers per second) and of near cryptographic
quality. I've modified it so that a given seed will generate the same
sequence on Intel, Alpha, RS6000, PowerPC, and MIPS architectures (and
probably anything else out there).  It's also been modified to generate
arbitrary length vectors of random numbers.  This makes generating numbers
more efficient because you can generate many numbers per function call.
MCell generates them in chunks of 10000 at a time.
*/
