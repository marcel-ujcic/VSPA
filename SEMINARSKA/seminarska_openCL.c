#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include "FreeImage.h"
#include <omp.h>
#include <CL/cl.h>

#define SIZE			(1024)
#define WORKGROUP_SIZE	(32,32)
#define MAX_SOURCE_SIZE	16384
#define COLOR_CHANNELS 4
int  NUM_THREADS =16; 
int num_of_clusters = 128; // 64;

void initCentroids(int *centroids, int num_of_clusters, unsigned char *image_in, int imageSize){
    int counter = 0;
    //Da lahko preverjamo pravilnost rezultatov, bomo cnetroide naredili z intervalom čez sliko
    printf("inicializacija centroidov velikost %d, gruce %d\n",imageSize,num_of_clusters);
    int interval = imageSize / num_of_clusters;
    printf("interval %d \n",interval);
    for(int i = 0; i < imageSize; i = i+interval){
        centroids[counter] = image_in[i*4];       //R
        centroids[counter + 1] = image_in[i*4+1]; //G
        centroids[counter + 2] = image_in[i*4+2]; //B
        counter +=4;
    }
   
}

void applyNewColoursToImage(unsigned char* image, int* closest_centroid_indices,  int size, int* centroids){
    //for each pixel in image assign it new centroid colour   
    for(int i = 0; i < (size); i=i+4){
        //find colour centroid for this pixel
    
        int closestCentroid = closest_centroid_indices[(int)i/4];
        
        //apply centroid colour to this pixel
        if (closestCentroid >=0 && closestCentroid < num_of_clusters){
            image[i] = centroids[closestCentroid * 4+0];          //R
            image[i+1] = centroids[closestCentroid * 4 + 1];    //G
            image[i+2] = centroids[closestCentroid * 4 + 2];    //B*/
            image[i+3] =255;
       } 
    }
}
int main(int argc, char *argv[])
{
   
   
    printf("%d \n",NUM_THREADS);
    omp_set_num_threads(NUM_THREADS);
    FIBITMAP *imageBitmap = FreeImage_Load(FIF_PNG, "input5.png", 0);
    FIBITMAP *imageBitmap32 = FreeImage_ConvertTo32Bits(imageBitmap);
    int width = FreeImage_GetWidth(imageBitmap32);
    int height = FreeImage_GetHeight(imageBitmap32);
    int pitch = FreeImage_GetPitch(imageBitmap32);

    //printf("velikost slike %d",width*height);
    unsigned char *image_in = (unsigned char *)malloc(height * pitch * sizeof(unsigned char));
    unsigned char *image_out = (unsigned char *)malloc(height * pitch * sizeof(unsigned char));
    //FreeImage_ConvertToRawBits(image_in, imageBitmap32, pitch, 32, 0xFF, 0xFF, 0xFF, TRUE);
    FreeImage_ConvertToRawBits(image_in, imageBitmap32, pitch, 32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK,FI_RGBA_BLUE_MASK, TRUE);
    //FreeImage_ConvertToRawBits(image_in_2, imageBitmap32, pitch, 32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK,FI_RGBA_BLUE_MASK, TRUE);
    int *centroids = (int*)malloc(num_of_clusters * 5 * sizeof(int));
    int interacije =100;
    int I =0;
    int *closest_centroid_indices = (int*)malloc(pitch * height * sizeof(int));
    int *farrest_centroid_indices = (int*)malloc(pitch * height * sizeof(int));
    int *rand_array=(int*)malloc(pitch * height * sizeof(int));

    double begin = omp_get_wtime();
    initCentroids(centroids,num_of_clusters,image_in,width*height);
    
  
    /*free(image_in);
    free(centroids);
    free(farrest_centroid_indices);
    free(closest_centroid_indices);*/
    size_t datasize = sizeof(cl_uchar) * height * pitch ;
    size_t cen_size = sizeof(cl_int)*num_of_clusters*4;
    size_t closes_cen_size= pitch * height * sizeof(cl_int);
    FILE *fp;
    char *source_str;
    size_t source_size;

    
    FILE *programHandle; // File that contains kernel1 functions
    size_t programSize;
    char *programBuffer;
    cl_program cpProgram;
    // 6 a: Read the OpenCL kernel1 from the source file and
    //      get the size of the kernel1 source
    programHandle = fopen("kernel1.cl", "r");
    if(!programHandle){
        printf("NI kernela\n");
    }
    fseek(programHandle, 0, SEEK_END);
    programSize = ftell(programHandle);
    rewind(programHandle);

    printf("Program size = %lu B \n", programSize);

    // 6 b: read the kernel1 source into the buffer programBuffer
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
    cl_int ret;
   

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

   
   /* const size_t szLocalWorkSize[2] = {32,32};
    const size_t szGlobalWorkSize[2] = {
        ((width - 1) / szLocalWorkSize[1] + 1) * szLocalWorkSize[1],
        ((height - 1) / szLocalWorkSize[0] + 1) * szLocalWorkSize[0]};*/

   const size_t szLocalWorkSize=num_of_clusters;
   const size_t szGlobalWorkSize=width*height*2;
	printf("global size = %d,%d \n",szGlobalWorkSize,szGlobalWorkSize);
    // Alcokacija pomnilnika na napravi za sliko in
    cl_mem GPU_image_in = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
									  datasize, image_in, &ret); // image in
		

    cl_mem GPU_image_out = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
									  datasize, NULL, &ret);// image out

    cl_mem GPU_centroids = clCreateBuffer(context, CL_MEM_READ_WRITE , 
									  cen_size, NULL, &ret);// centroids array

    cl_mem GPU_closest_centroids = clCreateBuffer(context, CL_MEM_READ_WRITE, 
									  closes_cen_size, NULL, &ret);// centroids array
   
    int size=height*pitch;
    // incializaci polja z naključnimi vrednostimi
    for (int i = 0; i < datasize; i++)
    {
      rand_array[i]=(rand() % datasize) * 4;
    }
    
    cl_mem GPU_rand = clCreateBuffer(context, CL_MEM_READ_ONLY, 
									  datasize, NULL, &ret);// centroids array
 
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
       
    cl_kernel kernel1 = clCreateKernel(program, "closestCentroid", &ret);
    cl_kernel kernel2 = clCreateKernel(program, "centroidAverage", &ret);
    cl_kernel kernel3 = clCreateKernel(program, "nanesinovebarve", &ret);
			// program, ime "s"cepca, napaka

	cl_event event;
	

    ret = clEnqueueWriteBuffer(
        command_queue,
        GPU_image_in,
        CL_FALSE,
        0,
        datasize,
        image_in,
        0,
        NULL,
        NULL);

    ret = clEnqueueWriteBuffer(
        command_queue,
        GPU_image_out,
        CL_FALSE,
        0,
        datasize,
        image_out,
        0,
        NULL,
        NULL);

    ret = clEnqueueWriteBuffer(
        command_queue,
        GPU_centroids,
        CL_FALSE,
        0,
        cen_size,
        centroids,
        0,
        NULL,
        NULL);
    ret = clEnqueueWriteBuffer(
        command_queue,
        GPU_closest_centroids,
        CL_FALSE,
        0,
        closes_cen_size,
        closest_centroid_indices,
        0,
        NULL,
        NULL);
    
    ret = clEnqueueWriteBuffer(
        command_queue,
        GPU_rand,
        CL_FALSE,
        0,
        datasize,
        rand_array,
        0,
        NULL,
        NULL);




   // clock_t begin = clock();
	// "s"cepec: zagon
  // "s"cepec: argumenti
 double start = omp_get_wtime();
    ret |= clSetKernelArg(kernel1, 0, sizeof(cl_mem), (void *)&GPU_image_in);
    ret |= clSetKernelArg(kernel1, 1, sizeof(cl_int), (void *)&width);
	ret |= clSetKernelArg(kernel1, 2, sizeof(cl_int), (void *)&height);
    ret |= clSetKernelArg(kernel1, 3, sizeof(cl_mem), (void *)&GPU_centroids);
    ret |= clSetKernelArg(kernel1, 4,sizeof(cl_mem), (void *)&GPU_closest_centroids);
    ret |= clSetKernelArg(kernel1, 5, sizeof(cl_int), (void *)&num_of_clusters);

    

    /*ret |= clSetKernelArg(kernel2, 0, sizeof(cl_mem), (void *)&GPU_centroids);
    ret |= clSetKernelArg(kernel2, 1, sizeof(cl_mem), (void *)&GPU_closest_centroids);
	ret |= clSetKernelArg(kernel2, 2, sizeof(cl_mem), (void *)&GPU_image_in);*/
    ret |= clSetKernelArg(kernel2, 3, sizeof(cl_int), (void *)&size);
    ret |= clSetKernelArg(kernel2, 4,sizeof(cl_int), (void *)&num_of_clusters);
    ret |= clSetKernelArg(kernel2, 5, sizeof(cl_mem), (void *)&GPU_rand);

    ret |= clSetKernelArg(kernel3, 0, sizeof(cl_mem), (void *)&GPU_image_out);
    /*ret |= clSetKernelArg(kernel3, 1, sizeof(cl_mem), (void *)&GPU_closest_centroids);
	ret |= clSetKernelArg(kernel3, 2, sizeof(cl_mem), (void *)&GPU_centroids);*/
    ret |= clSetKernelArg(kernel3, 3, sizeof(cl_int), (void *)&num_of_clusters);
    ret |= clSetKernelArg(kernel3, 4,sizeof(cl_int), (void *)&size);



   
   
                         
	// branje v pomnilnik iz naparave, 0 = offset
	// zadnji trije - dogodki, ki se morajo zgoditi prej

   
    for (int i= 0; i < interacije; i++)
    {
        ret = clEnqueueNDRangeKernel(command_queue, kernel1, 1, NULL,						
							 &szGlobalWorkSize, NULL, 0, NULL, NULL);

        ret = clEnqueueNDRangeKernel(command_queue, kernel2, 1, NULL,						
								 &szLocalWorkSize, NULL, 0, NULL,NULL);
    }
     
    ret = clEnqueueNDRangeKernel(command_queue, kernel3, 1, NULL,						
								 &szGlobalWorkSize, NULL, 0, NULL, NULL);
  //  double vmesni_cas= omp_get_wtime();
  /*  ret = clEnqueueReadBuffer(command_queue, GPU_centroids, CL_TRUE, 0,						
                                cen_size, centroids, 0, NULL, NULL);

    ret = clEnqueueReadBuffer(command_queue, GPU_closest_centroids, CL_TRUE, 0,						
                                closes_cen_size, closest_centroid_indices, 0, NULL, NULL);
    ret = clEnqueueReadBuffer(command_queue, GPU_image_out, CL_TRUE, 0,						
                                datasize, image_out, 0, NULL, NULL);*/
    double vmesni_cas1= omp_get_wtime();
    //applyNewColoursToImage(image_out, closest_centroid_indices,  pitch*height, centroids);
    double end = omp_get_wtime();
    ret = clEnqueueReadBuffer(command_queue, GPU_image_out, CL_TRUE, 0,						
                                datasize, image_out, 0, NULL, NULL);
    /*for (int  i = 0; i < datasize; i++)
    {
        printf("%s \n",image_out[i]);
    }*/
    
    //find edges
    double cpu_start= clock();
    double cpu_end = clock();

  /*
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

    
    double time_spent_cpu = (cpu_end - cpu_start)/ CLOCKS_PER_SEC;*/

    cl_ulong time_start;
    cl_ulong time_end;
 

    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);

    double nanoSeconds = time_end - time_start;
    printf("kernel1 Execution time is: %0.6f seconds \n", (vmesni_cas1-start)+(end-vmesni_cas1));


    //printf("Time consumed on CPU: %f, GPU: %lfs  \n",time_spent_cpu,time_spent);
    //save output image
   FIBITMAP *dst = FreeImage_ConvertFromRawBits(image_out, width, height, pitch,
		32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, TRUE);
    printf("Shranujem sliko \n");
    FreeImage_Save(FIF_PNG, dst, "output_GPU.png", 0);


    ret = clFlush(command_queue);
    ret = clFinish(command_queue);
    ret = clReleaseKernel(kernel1);
    ret = clReleaseKernel(kernel2);
    ret = clReleaseKernel(kernel3);
    ret = clReleaseProgram(program);
    ret = clReleaseMemObject(GPU_image_in);
    ret = clReleaseMemObject(GPU_image_out);
    ret = clReleaseMemObject(GPU_closest_centroids);
    ret = clReleaseMemObject(GPU_centroids);
   //ret = clReleaseMemObject(b_mem_obj);
    ret = clReleaseCommandQueue(command_queue);
    ret = clReleaseContext(context);
	free(image_in);
    free(centroids);
    free(farrest_centroid_indices);
    free(closest_centroid_indices);
    
    return 0;
}

