//definition of local size¸
#define LOCAL_SIZE 16
#define CHUNK_SIZE (LOCAL_SIZE+2)



inline short4 getIntensity4(__global uchar4 *image, int y, int x, int height,
                            int width, int cpp) {
  if (x < 0 || x >= width)
    return (short4)(0);
  if (y < 0 || y >= height)
    return (short4)(0);
  return convert_short4(image[y * width + x]);
}

inline int getPixel(__global unsigned char *image, int y, int x, int width, int height)
{
    if (x < 0 || x >= width)
        return 0;
    if (y < 0 || y >= height)
        return 0;
    return image[y * width + x];
}


__kernel void sobel(__global unsigned char *image_in,
                                      __global unsigned char *image_out, int width,
                                      int height) {

    char  Gx, Gy;
    float tempPixel;
    int g_x = get_global_id(0);
    int g_y = get_global_id(1);
   
    int lx = get_local_id(0) +1;
    int ly = get_local_id(1) +1 ;

    size_t space = g_y * width + g_x;

     __local unsigned char LBLOCK[CHUNK_SIZE][CHUNK_SIZE];
    
    if (g_x < width && g_y < height) {
       
        LBLOCK[lx][ly] = image_in[space];

        // levo
        if (lx == 1){
             
               LBLOCK[0][ly] = image_in[space - 1];
        }
        // desno
        else if (lx == LOCAL_SIZE)
        
            LBLOCK[LOCAL_SIZE + 1][ly] = image_in[space + 1];

        // vrh
        if (ly == 1) {
            LBLOCK[lx][0] = image_in[space - width];
            // vrh levo
            if (lx == 1)
                LBLOCK[0][0] = image_in[space - width - 1];
            // vrh desno
            else if (lx == LOCAL_SIZE)
                LBLOCK[LOCAL_SIZE + 1][0] = image_in[space - width + 1];
        }
        // dno
        if (ly == LOCAL_SIZE) {
            LBLOCK[lx][LOCAL_SIZE + 1] = image_in[space + width];
            // dno levo
            if (lx == 1){
                
                LBLOCK[0][LOCAL_SIZE + 1] = image_in[space + width - 1];
            }
            // dno
            else if (lx == LOCAL_SIZE)
                
                LBLOCK[LOCAL_SIZE + 1][LOCAL_SIZE + 1] = image_in[space + width + 1];
        }
    }
       
      barrier(CLK_LOCAL_MEM_FENCE );
   
    if (g_x < width && g_y < height) {
         
       
       Gx = -LBLOCK[lx - 1][ly - 1] - 2* LBLOCK[lx - 1][ly] -
          LBLOCK[lx - 1][ly + 1] + LBLOCK[lx + 1][ly - 1]- +
          2 * LBLOCK[lx + 1][ly] + LBLOCK[lx + 1][ly + 1];
  
       Gy = -LBLOCK[lx - 1][ly - 1] - 2 * LBLOCK[lx][ly - 1] -
            LBLOCK[lx + 1][ly - 1] + LBLOCK[lx - 1][ly + 1] +
            2 * LBLOCK[lx][ly + 1] + LBLOCK[lx + 1][ly + 1];
     
        barrier(CLK_LOCAL_MEM_FENCE);

        tempPixel = sqrt((float)(Gx * Gx + Gy * Gy));
      
        if (tempPixel > 255)
            image_out[space] = 255;
        else
            image_out[space] = tempPixel;
    }

}
__kernel void sobel_GPU(__global unsigned char *imageIn,
                                      __global unsigned char *image_out, int width,
                                      int height){
                                      
    int lx, ly;
    int Gx, Gy;
    int tempPixel;
    lx = get_global_id(1);
    ly=get_global_id(0);

    //printf("X= %d, y= %d \n",lx,ly); 
    Gx = -getPixel(imageIn, lx - 1, ly - 1, width, height) - 2 * getPixel(imageIn, lx - 1, ly, width, height) -
          getPixel(imageIn, lx - 1, ly + 1, width, height) + getPixel(imageIn, lx + 1, ly - 1, width, height) +
          2 * getPixel(imageIn, lx + 1, ly, width, height) + getPixel(imageIn, lx + 1, ly + 1, width, height);
    Gy = -getPixel(imageIn, lx - 1, ly - 1, width, height) - 2 * getPixel(imageIn, lx, ly - 1, width, height) -
          getPixel(imageIn, lx + 1, ly - 1, width, height) + getPixel(imageIn, lx - 1, ly + 1, width, height) +
          2 * getPixel(imageIn, lx, ly + 1, width, height) + getPixel(imageIn, lx + 1, ly + 1, width, height);

    tempPixel = sqrt((float)(Gx * Gx + Gy * Gy));
    printf("%d",tempPixel);
    if (tempPixel > 255)
        image_out[lx * width + ly] = 255;
    else
        image_out[lx * width + ly] = tempPixel;
    //printf("vrednost pixla %d \n",image_out[lx * width + ly]);
            
}
