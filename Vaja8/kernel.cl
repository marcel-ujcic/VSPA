#define SIZE 256



__kernel void hist(__global unsigned char *imageIn,  __global unsigned int *blue_out, __global unsigned int *red_out, __global unsigned int  *green_out, const int width, const int height)
{
    int i = get_global_id(0);
    int li = get_local_id(0);

	
	

    __local unsigned int buffer_blue[SIZE];
	__local unsigned int buffer_red[SIZE];
	__local unsigned int buffer_green[SIZE];
	
	if(i<width*height){
		atomic_inc(&buffer_blue[imageIn[i  * 4]]);
	atomic_inc(&buffer_green[imageIn[i  * 4 + 1]]);
	atomic_inc(&buffer_red[imageIn[i * 4 + 2]]);

	barrier(CLK_LOCAL_MEM_FENCE);

	atomic_add(&blue_out[li],buffer_blue[li]);
	atomic_add(&green_out[li],buffer_green[li]);
	atomic_add(&red_out[li],buffer_red[li]);

barrier(CLK_LOCAL_MEM_FENCE);
	}
	
	  //Each color channel is 1 byte long, there are 4 channels BLUE, GREEN, RED and ALPHA
    //The order is BLUE|GREEN|RED|ALPHA for each pixel, we ignore the ALPHA channel when computing the histograms
	

	
	// GLOBAL TEST
	/*		atomic_inc(&blue_out[imageIn[i  * 4]]);
			atomic_inc(&green_out[imageIn[i* 4 + 1]]);
			atomic_inc(&red_out[imageIn[i  * 4 + 2]]);*/
}