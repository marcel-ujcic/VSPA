#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include "FreeImage.h"
#include </usr/include/CL/cl.h>




#define SIZE			(1024)
#define WORKGROUP_SIZE	(32,32)
#define MAX_SOURCE_SIZE	16384
#define COLOR_CHANNELS 4
int getPixel(unsigned char *image, int y, int x, int width, int height)
{
    if (x < 0 || x >= width)
        return 0;
    if (y < 0 || y >= height)
        return 0;
    return image[y * width + x];
}

void sobelCPU(unsigned char *image_in, unsigned char *image_out, int width, int height)
{
    int i, j;
    int Gx, Gy;
    int tempPixel;

    //za vsak piksel v sliki
    for (i = 0; i < height; i++)
        for (j = 0; j < width; j++)
        {
            Gx = -getPixel(image_in, i - 1, j - 1, width, height) - 2 * getPixel(image_in, i - 1, j, width, height) -
                 getPixel(image_in, i - 1, j + 1, width, height) + getPixel(image_in, i + 1, j - 1, width, height) +
                 2 * getPixel(image_in, i + 1, j, width, height) + getPixel(image_in, i + 1, j + 1, width, height);
            Gy = -getPixel(image_in, i - 1, j - 1, width, height) - 2 * getPixel(image_in, i, j - 1, width, height) -
                 getPixel(image_in, i + 1, j - 1, width, height) + getPixel(image_in, i - 1, j + 1, width, height) +
                 2 * getPixel(image_in, i, j + 1, width, height) + getPixel(image_in, i + 1, j + 1, width, height);
            tempPixel = sqrt((float)(Gx * Gx + Gy * Gy));
            if (tempPixel > 255)
                image_out[i * width + j] = 255;
            else
                image_out[i * width + j] = tempPixel;
        }
}

int main(int argc, char *argv[])
{
    
    //Load image from file
    FIBITMAP *imageBitmap = FreeImage_Load(FIF_PNG, "input6.png", 0);
    //Convert it to an 8-bit grayscale image
    FIBITMAP *imageBitmap8 = FreeImage_ConvertTo8Bits(imageBitmap);


    //Get image dimensions
    int width = FreeImage_GetWidth(imageBitmap8);
    int height = FreeImage_GetHeight(imageBitmap8);
    int pitch = FreeImage_GetPitch(imageBitmap8);
    int cpp=COLOR_CHANNELS;

    printf("Sirina: %d  visina: %d  \n",width,height);
    //Preapare room for a raw data copy of the image
    unsigned char *image_in = (unsigned char *)malloc(height * pitch * sizeof(unsigned char));
    FreeImage_ConvertToRawBits(image_in, imageBitmap8, pitch, 8, 0xFF, 0xFF, 0xFF, TRUE);

    unsigned char *image_out = (unsigned char *)malloc(height * pitch * sizeof(unsigned char));
    size_t datasize = sizeof(cl_uchar) * height * pitch ;
    char ch;
    //int i;
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
    programHandle = fopen("kernel0.cl", "r");
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

   
    const size_t szLocalWorkSize[2] = {16, 16};
    const size_t szGlobalWorkSize[2] = {
        ((width - 1) / szLocalWorkSize[1] + 1) * szLocalWorkSize[1],
        ((height - 1) / szLocalWorkSize[0] + 1) * szLocalWorkSize[0]};

	printf("global size = %d,%d \n",szGlobalWorkSize[0],szGlobalWorkSize[1]);
    // Alokacija pomnilnika na napravi za sliko in
    cl_mem a_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
									  datasize, image_in, &ret); // image in
		

    cl_mem c_mem_obj = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
									  datasize, NULL, &ret);// image out
  
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
    cl_kernel kernel = clCreateKernel(program, "sobel", &ret);
			// program, ime "s"cepca, napaka

 
  

	cl_event event;
	

    ret = clEnqueueWriteBuffer(
        command_queue,
        a_mem_obj,
        CL_FALSE,
        0,
        datasize,
        image_in,
        0,
        NULL,
        NULL);

    clock_t begin = clock();
	// "s"cepec: zagon
  // "s"cepec: argumenti
    ret = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&a_mem_obj);
   // ret |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&b_mem_obj);
    ret |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&c_mem_obj);
    ret |= clSetKernelArg(kernel, 2, sizeof(cl_int), (void *)&width);
	ret |= clSetKernelArg(kernel, 3, sizeof(cl_int), (void *)&height);
   // ret |= clSetKernelArg(kernel, 4, sizeof(cl_int), (void *)&cpp);
			// "s"cepec, "stevilka argumenta, velikost podatkov, kazalec na podatke


    ret = clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL,						
								 szGlobalWorkSize, szLocalWorkSize, 0, NULL, &event);	
			// vrsta, "s"cepec, dimenzionalnost, mora biti NULL, 
			// kazalec na "stevilo vseh niti, kazalec na lokalno "stevilo niti, 
			// dogodki, ki se morajo zgoditi pred klicem
	clWaitForEvents(1, &event);
			  clock_t end = clock();																			
    // Kopiranje rezultatov
    ret = clEnqueueReadBuffer(command_queue, c_mem_obj, CL_TRUE, 0,						
							 datasize, image_out, 0, NULL, NULL);				
			// branje v pomnilnik iz naparave, 0 = offset
			// zadnji trije - dogodki, ki se morajo zgoditi prej


	

	//rezerviramo prostor za sliko (RGBA)
	unsigned char *image = (unsigned char *)malloc(height * width * sizeof(unsigned char) * 4);
    

    
    //find edges
    double cpu_start= clock();
    sobelCPU(image_in, image_out, width, height);
    double cpu_end = clock();

  
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

    
    double time_spent_cpu = (cpu_end - cpu_start)/ CLOCKS_PER_SEC;

    cl_ulong time_start;
    cl_ulong time_end;

    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);

    double nanoSeconds = time_end - time_start;
    printf("Kernel Execution time is: %0.6f milliseconds \n", nanoSeconds / 1000000.0);


    printf("Time consumed on CPU: %f, GPU: %lfs  \n",time_spent_cpu,time_spent);
    //save output image
    FIBITMAP *dst = FreeImage_ConvertFromRawBits(image_out, width, height, pitch,
		8, 0xFF, 0xFF, 0xFF, TRUE);
	FreeImage_Save(FIF_PNG, dst, "robovi.png", 0);


    ret = clFlush(command_queue);
    ret = clFinish(command_queue);
    ret = clReleaseKernel(kernel);
    ret = clReleaseProgram(program);
    ret = clReleaseMemObject(a_mem_obj);
   // ret = clReleaseMemObject(b_mem_obj);
    ret = clReleaseMemObject(c_mem_obj);
    ret = clReleaseCommandQueue(command_queue);
    ret = clReleaseContext(context);
	
    /*free(A);
    free(B);
    free(C);*/
    return 0;
}


/*
PRIMERJANJE REZILTATOV

size 640*480 -> CPU = 0.003348s  GPU = 0.000341   pohitritev 9.8x
size 800*600 -> CPU = 0.005209s  GPU =  0.000718s pohitritev 7.3x
size 1600*900 -> CPU = 0.015822s  GPU =  0.001143s  pohitritev 13.8x
size 1920*1080 -> CPU = 0.023336s  GPU =  0.001545s  pohitritev 15.1x
size 3840*2160 -> CPU = 0.098865s  GPU =  0.009582s  pohitritev 10.3x

*/
