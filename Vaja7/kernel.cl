#define LOCAL_SIZE 16


inline int getPixel(__global unsigned char *image, int y, int x, int width, int height)
{
    if (x < 0 || x >= width)
        return 0;
    if (y < 0 || y >= height)
        return 0;
    return image[y * width + x];
}


// kernel
__kernel void sobel(__global unsigned char *image_in, __global unsigned char *image_out,  int width,  int height) {

   
    
    int Gx, Gy, tempPixel;
    int g_x = get_global_id(1);
    int g_y = get_global_id(0);
   
    int lx = get_local_id(1) ;
    int ly = get_local_id(0) ;

    const size_t g_id = g_x * width + g_y;

    __local unsigned char LBLOCK[(LOCAL_SIZE + 2)][(LOCAL_SIZE + 2)];
    //if(g_y>20)
        printf("%d,%d \n",g_x,g_y);
    //if (g_x < width && g_y < height) {
        LBLOCK[lx][ly] = image_in[g_id];

        // left
        if (lx == 1)
            LBLOCK[0][ly] = image_in[g_id - 1];
        // right
        else if (lx == LOCAL_SIZE)
            LBLOCK[LOCAL_SIZE + 1][ly] = image_in[g_id + 1];

        // top
        if (ly == 1) {
            LBLOCK[lx][0] = image_in[g_id - width];

            // top-left
            if (lx == 1)
                LBLOCK[0][0] = image_in[g_id - width - 1];
            // top-right
            else if (lx == LOCAL_SIZE)
                LBLOCK[LOCAL_SIZE + 1][0] = image_in[g_id - width + 1];
        }

        // bottom
        if (ly == LOCAL_SIZE) {
            LBLOCK[lx][LOCAL_SIZE + 1] = image_in[g_id + width];

            // bottom-left
            if (lx == 1)
                LBLOCK[0][LOCAL_SIZE + 1] = image_in[g_id + width - 1];
            // bottom-right
            else if (lx == LOCAL_SIZE)
                LBLOCK[LOCAL_SIZE + 1][LOCAL_SIZE + 1] = image_in[g_id + width + 1];
        }
    //}
    
    barrier(CLK_LOCAL_MEM_FENCE);
      
     
    if (g_x < width && g_y < height) {
       
       int  Gx = -LBLOCK[lx - 1][ly - 1] - 2 * LBLOCK[lx - 1][ly] - LBLOCK[lx - 1][ly + 1] +
            LBLOCK[lx + 1][ly - 1] + 2 * LBLOCK[lx + 1][ly] + LBLOCK[lx + 1][ly + 1];
        int Gy = -LBLOCK[lx - 1][ly - 1] - 2 * LBLOCK[lx][ly - 1] - LBLOCK[lx + 1][ly - 1] +
            LBLOCK[lx - 1][ly + 1] + 2 * LBLOCK[lx][ly + 1] + LBLOCK[lx + 1][ly + 1];
        tempPixel = sqrt((float)(Gx * Gx + Gy * Gy));
        if (tempPixel > 255)
            image_out[g_id] = 255;
        else
            image_out[g_id] = tempPixel;
    }
}