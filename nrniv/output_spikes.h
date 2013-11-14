#ifndef output_spikes_h
#define output_spikes_h

void output_spikes(void);
void mk_spikevec_buffer(int);

extern int spikevec_buffer_size;
extern int spikevec_size;
extern double* spikevec_time;	
extern int* spikevec_gid;

#endif
