


__kernel void closestCentroid(__global unsigned char *image_in, int width, int height,
     __global int *centroids, __global int * closest_centroids_indices,int num_clusters){
        int Gi = get_global_id(0);
        if(Gi < (width*height)*2){
            int imageStartingPointIndex = Gi * 4;
            int red = image_in[imageStartingPointIndex];
            int green = image_in[imageStartingPointIndex + 1];
            int blue = image_in[imageStartingPointIndex + 2];

            
            int centroidIndex = 0;
            int minimum_distance=99;
            for(int i = 4; i < (num_clusters * 4); i = i + 4){
                float current_distance =  sqrt(pow((centroids[i] - red),2.0)+ pow((centroids[i+1] - green),2.0)+ pow((centroids[i+2] - blue),2.0));
                if(current_distance < minimum_distance){
                    centroidIndex = i / 4;
                    minimum_distance = current_distance;
                }
            }       
            closest_centroids_indices[Gi]=centroidIndex;
        }
}


__kernel void centroidAverage(__global int *centroids,
__global int *closest_centroid_indices, __global unsigned char *image_in,
 int size, int num_centroids,__global int *rand){
   
    int Gi = get_global_id(0);
  
        
    if(Gi < num_centroids){
        int count = 0;
        int red = 0;
        int green = 0;
        int blue = 0;
        for(int i=0; i< size;i++){
           
            if(closest_centroid_indices[i] == Gi){
            int strindex = i*4;
            count++;
            red = red + image_in[strindex];
            green = green + image_in[strindex+ 1];
            blue = blue + image_in[strindex + 2];
            }
        }
        if(count ==0){
            int randomPixel = rand[Gi];
            centroids[Gi * 4] = image_in[randomPixel];
            centroids[Gi * 4 + 1] = image_in[randomPixel];
            centroids[Gi * 4 + 2] = image_in[randomPixel];
             centroids[Gi * 4 + 3] = 255;
        }else{
           
            centroids[Gi * 4] = red / count;
            centroids[Gi * 4 +1] = green / count;
            centroids[Gi * 4 +2] = blue / count;
            centroids[Gi * 4 + 3] = image_in[Gi * 4 + 3];
        }
    }


}
__kernel void nanesinovebarve(__global unsigned char *image_out, 
__global int *closest_centroids_indices, __global int *centroids, int num_clusters, int size){
    int Gi = get_global_id(0);
    //printf("kernel \n");
    if(Gi < size){
        int closestCentroid = closest_centroids_indices[(int)Gi];
        if( closestCentroid >=0 && closestCentroid< num_clusters){
            image_out[Gi]=centroids[closestCentroid*4 ];  //R
          
        }

    }
}