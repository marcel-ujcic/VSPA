#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FreeImage.h"
#include </usr/include/CL/cl.h>
#include <time.h>
#define BINS 256


struct histogram {
	unsigned int *R;
	unsigned int *G;
	unsigned int *B;
};

void histogramCPU(unsigned char *imageIn, histogram H, int width, int height)
{

    //Each color channel is 1 byte long, there are 4 channels BLUE, GREEN, RED and ALPHA
    //The order is BLUE|GREEN|RED|ALPHA for each pixel, we ignore the ALPHA channel when computing the histograms
	for (int i = 0; i < (height); i++)
		for (int j = 0; j < (width); j++)
		{
			H.B[imageIn[(i * width + j) * 4]]++;
			H.G[imageIn[(i * width + j) * 4 + 1]]++;
			H.R[imageIn[(i * width + j) * 4 + 2]]++;
		}
}
void printHistogram(histogram H) {
	printf("Colour\tNo. Pixels\n");
	for (int i = 0; i < BINS; i++) {
		if (H.B[i]>0)
			printf("%dB\t%d\n", i, H.B[i]);
		if (H.G[i]>0)
			printf("%dG\t%d\n", i, H.G[i]);
		if (H.R[i]>0)
			printf("%dR\t%d\n", i, H.R[i]);
	}

}
int main(void)
{

    //Load image from file
	FIBITMAP *imageBitmap = FreeImage_Load(FIF_BMP, "test.bmp", 0);
	//Convert it to a 32-bit image
    FIBITMAP *imageBitmap32 = FreeImage_ConvertTo32Bits(imageBitmap);
	
    //Get image dimensions
    int width = FreeImage_GetWidth(imageBitmap32);
	int height = FreeImage_GetHeight(imageBitmap32);
	int pitch = FreeImage_GetPitch(imageBitmap32);
	//Preapare room for a raw data copy of the image
    unsigned char *imageIn = (unsigned char *)malloc(height*pitch * sizeof(unsigned char));
    //Initalize the histogram
    histogram H;
	H.B = (unsigned int*)calloc(BINS, sizeof(unsigned int));
	H.G = (unsigned int*)calloc(BINS, sizeof(unsigned int));
	H.R = (unsigned int*)calloc(BINS, sizeof(unsigned int));
	

	//initialize the histohram for GPU compute
	histogram H_gpu;
	H_gpu.B = (unsigned int*)calloc(BINS, sizeof(unsigned int));
	H_gpu.G = (unsigned int*)calloc(BINS, sizeof(unsigned int));
	H_gpu.R = (unsigned int*)calloc(BINS, sizeof(unsigned int));
	

    //Extract raw data from the image
	FreeImage_ConvertToRawBits(imageIn, imageBitmap32, pitch, 32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, TRUE);

    //Free source image data
	FreeImage_Unload(imageBitmap32);
	FreeImage_Unload(imageBitmap);

    //Compute and print the histogram
	double cpu_start= clock();
    
	histogramCPU(imageIn, H, width, height);
	double cpu_end = clock();
	printf("width = %d, height=%d \n",width,height);
	//printHistogram(H); //PRINTOUT HISTOGRAM 


	cl_int ret;

	int vectorSize = width*height;

    // Branje datoteke
    FILE *fp;
    char *source_str;
    size_t source_size;

    
    FILE *programHandle; // File that contains kernel functions
    size_t programSize;
    char *programBuffer;
    cl_program cpProgram;
    // 6 a: Read the OpenCL kernel from the source file and
    //      get the size of the kernel source
    programHandle = fopen("kernel.cl", "r");
	if (!programHandle)
	{
		printf("Error: failed to open kernel");
		return EXIT_FAILURE;    
	}
	  
    fseek(programHandle, 0, SEEK_END);
    programSize = ftell(programHandle);
    rewind(programHandle);

    printf("Program size = %lu B \n", programSize);

    // 6 b: read the kernel source into the buffer programBuffer
    //      add null-termination-required by clCreateProgramWithSource
	
	programBuffer = (char *)malloc(programSize + 1);

    programBuffer[programSize] = '\0'; // add null-termination
    fread(programBuffer, sizeof(char), programSize, programHandle);
    fclose(programHandle);

	
    // Podatki o platformi
    cl_platform_id	platform_id[10];
    cl_uint			ret_num_platforms;
	char			*buf;
	size_t			buf_len;
	
	ret = clGetPlatformIDs(10, platform_id, &ret_num_platforms);
			// max. "stevilo platform, kazalec na platforme, dejansko "stevilo platform
	
	// Podatki o napravi
	cl_device_id	device_id[10];
	cl_uint			ret_num_devices;
	// Delali bomo s platform_id[0] na GPU
	ret = clGetDeviceIDs(platform_id[0], CL_DEVICE_TYPE_GPU, 10,	
						 device_id, &ret_num_devices);				
			// izbrana platforma, tip naprave, koliko naprav nas zanima
			// kazalec na naprave, dejansko "stevilo naprav

    // Kontekst
	
    cl_context context = clCreateContext(NULL, 1, &device_id[0], NULL, NULL, &ret);
	if(!context){
		printf("Error: FIled to create a context");
		return EXIT_FAILURE;
	}
			// kontekst: vklju"cene platforme - NULL je privzeta, "stevilo naprav, 
			// kazalci na naprave, kazalec na call-back funkcijo v primeru napake
			// dodatni parametri funkcije, "stevilka napake
 
    // Ukazna vrsta
    cl_command_queue command_queue = clCreateCommandQueue(context, device_id[0], 0, &ret);
			// kontekst, naprava, INORDER/OUTOFORDER, napake
			if (!command_queue)
    {
        printf("Error: Failed to create a command Q!\n");
        return EXIT_FAILURE;
    }

	// Delitev dela
	
  /*  size_t local_item_size[2] ={32,32} ;
	size_t num_groups_1 = ((width-1)/local_item_size[1]+1);		
	size_t num_groups_0 = ((height-1)/local_item_size[0]+1);	
    size_t global_item_size[2] = {num_groups_0*local_item_size[0],num_groups_1*local_item_size[1]};
    */
    const size_t szLocalWorkSize = BINS;
    const size_t szGlobalWorkSize = 
        (((width - 1) / szLocalWorkSize - 1) * szLocalWorkSize);
	const size_t global_itm_size = szLocalWorkSize*szGlobalWorkSize;

	printf("global size = %d,%d \n",szGlobalWorkSize,global_itm_size);
	size_t datasize= BINS*sizeof(unsigned int);
	int imageSize = height * width * sizeof(unsigned char) * 4;
    // Alokacija pomnilnika na napravi za sliko in
    cl_mem a_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
									  imageSize, imageIn, &ret); // image in
		

    cl_mem blue_out = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
									  datasize , NULL, &ret);// hist blue out

  	cl_mem red_out = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
									  datasize , NULL, &ret);// hist red out

  	cl_mem green_out = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
									  datasize , NULL, &ret);// hist green out
  
    // Priprava programa
   
    cl_program program = clCreateProgramWithSource(context,	1, (const char **)&programBuffer,  
												   NULL, &ret);
			// kontekst, "stevilo kazalcev na kodo, kazalci na kodo,		
			// stringi so NULL terminated, napaka													
 
    // Prevajanje
    ret = clBuildProgram(program, 1, &device_id[0], NULL, NULL, NULL);
			// program, "stevilo naprav, lista naprav, opcije pri prevajanju,
			// kazalec na funkcijo, uporabni"ski argumenti

	// Log
	size_t build_log_len;
	char *build_log;
	ret = clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG, 
								0, NULL, &build_log_len);
			// program, "naprava, tip izpisa, 
			// maksimalna dol"zina niza, kazalec na niz, dejanska dol"zina niza
	build_log =(char *)malloc(sizeof(char)*(build_log_len+1));
	ret = clGetProgramBuildInfo(program, device_id[0], CL_PROGRAM_BUILD_LOG, 
							    build_log_len, build_log, NULL);
	printf("%s\n", build_log);
	free(build_log);

    // "s"cepec: priprava objekta
    cl_kernel kernel = clCreateKernel(program, "hist", &ret);
			// program, ime "s"cepca, napaka

 
  

	cl_event event;
	

    ret = clEnqueueWriteBuffer(
        command_queue,
        a_mem_obj,
        CL_FALSE,
        0,
        datasize,
        imageIn,
        0,
        NULL,
        NULL);

    
	// "s"cepec: zagon
  // "s"cepec: argumenti
    ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&a_mem_obj);
   // ret |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&b_mem_obj);
    ret |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&blue_out);
	 ret |= clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&red_out);
	  ret |= clSetKernelArg(kernel, 3, sizeof(cl_mem), (void *)&green_out);
    ret |= clSetKernelArg(kernel, 4, sizeof(cl_int), (void *)&width);
	ret |= clSetKernelArg(kernel, 5, sizeof(cl_int), (void *)&height);
   // ret |= clSetKernelArg(kernel, 4, sizeof(cl_int), (void *)&cpp);
			// "s"cepec, "stevilka argumenta, velikost podatkov, kazalec na podatke

	double gpu_s = clock();
    ret = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL,						
								 &global_itm_size, &szLocalWorkSize, 0, NULL, &event);	
			// vrsta, "s"cepec, dimenzionalnost, mora biti NULL, 
			// kazalec na "stevilo vseh niti, kazalec na lokalno "stevilo niti, 
			// dogodki, ki se morajo zgoditi pred klicem
	clWaitForEvents(1, &event);
	double gpu_e = clock();	
																					
    // Kopiranje rezultatov
  	ret = clEnqueueReadBuffer(command_queue, blue_out, CL_TRUE, 0,						
							 datasize, H_gpu.B , 0, NULL, NULL);	

	ret = clEnqueueReadBuffer(command_queue, red_out, CL_TRUE, 0,						
							 datasize, H_gpu.R , 0, NULL, NULL);
	
	ret = clEnqueueReadBuffer(command_queue, green_out, CL_TRUE, 0,						
							 datasize, H_gpu.G , 0, NULL, NULL);	
			// branje v pomnilnik iz naparave, 0 = offset
			// zadnji trije - dogodki, ki se morajo zgoditi prej
	printf("Used time on CPU: %0.5f \n",(cpu_end-cpu_start)/CLOCKS_PER_SEC);
	printf("Time used on GPU: %0.5f \n",(gpu_e-gpu_s)/ CLOCKS_PER_SEC);

	
	//printHistogram(H_gpu);

 	ret = clFlush(command_queue);
    ret = clFinish(command_queue);
    ret = clReleaseKernel(kernel);
    ret = clReleaseProgram(program);
    ret = clReleaseMemObject(a_mem_obj);
   // ret = clReleaseMemObject(b_mem_obj);
    ret = clReleaseMemObject(blue_out);
	 ret = clReleaseMemObject(red_out);
	ret = clReleaseMemObject(green_out);
    ret = clReleaseCommandQueue(command_queue);
    ret = clReleaseContext(context);
	
	


	return 0;

}


/*
ALL TIME IS IN SECONDS
width = 640, height=480 	Used time on CPU: 0.00119 	Time used on GPU: 0.00028 	speed up: 4.25x

width = 800, height=600  	Used time on CPU: 0.00167 	Time used on GPU: 0.00021 	speed up: 7.95x

width = 1600, height=900 	Used time on CPU: 0.00227 	Time used on GPU: 0.00029	speed up: 7.83x

width = 1920, height=1080 	Used time on CPU: 0.00327 	Time used on GPU: 0.00036  	speed up:9.08x

width = 3840, height=2160 	Used time on CPU: 0.01282 	Time used on GPU: 0.00044 	speed up:29.14x



*/