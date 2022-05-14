

__kernel void main(__global int *centroids,
__global int *closest_centroid_indices, __global unsigned char *image_in, int size, int num_centroids){

    int Gi = get_global_id(0);
    if(Gi < num_centroids){

        int count = 0;
        int red = 0;
        int green = 0;
        int blue = 0;

        for(int i=0; i< size;i++){
            if(closest_centroid_indices[i] == Gi){
            int strindex = i*3;
            count++;
            red = red + image_in[strindex];
            green = green + image_in[strindex+ 1];
            blue = blue + image_in[strindex + 2];
            }
        }
        if(count ==0){
            int randomPixel = (rand() % size) * 4;
            centroids[Gi * 3] = image_in[randomPixel];
            centroids[Gi * 3 + 1] = image_in[randomPixel];
            centroids[Gi * 3 + 2] = image_in[randomPixel];
        }else{
            centroids[Gi * 3] = red / count;
            centroids[Gi * 3 + 1] = green / count;
            centroids[Gi * 3 + 2] = blue / count;
        }
    }


}