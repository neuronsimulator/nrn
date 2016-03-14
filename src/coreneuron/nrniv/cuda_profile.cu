#include "cuda_profiler_api.h"

void start_cuda_profile() {
	cudaProfilerStart();
}

void stop_cuda_profile() {
	cudaProfilerStop();
}

