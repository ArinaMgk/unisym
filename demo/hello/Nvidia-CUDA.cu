// ASCII CPP-NVCC
// : test on Visual Studio 2022
// $ nvcc kernel.cu -o ciallo # you may need this for Linux

// Optional if the suffix of the document is `.cu`
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <stdio.h>

__global__ void ciallo()
{
	printf("Ciallo~ CUDA~ @dosconio, 20240418\n");
}

int main() 
{
	ciallo <<<2, 2>>> ();// the `ciallo` will be called for 2*2 times!
	cudaDeviceSynchronize();
}

