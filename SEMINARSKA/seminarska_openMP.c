#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include "FreeImage.h"
#include <omp.h>
#include </usr/include/CL/cl.h>

#define SIZE			(1024)
#define WORKGROUP_SIZE	(32,32)
#define MAX_SOURCE_SIZE	16384
#define COLOR_CHANNELS 4
int  NUM_THREADS =2; 
int num_of_clusters = 64; // 64;
float razdalja(float x1, float x2, float x3)
{
    return sqrt(pow(x1, 2.0) + pow(x2, 2.0) + pow(x3, 2.0));
}


//#pragma omp atomic
void initCentroids(int *centroids, int num_of_clusters, unsigned char *image_in, int imageSize){
    int counter = 0;
    //Da lahko preverjamo pravilnost rezultatov, bomo cnetroide naredili z intervalom čez sliko
    printf("inicializacija centroidov velikost %d, gruce %d\n",imageSize,num_of_clusters);
    int interval = imageSize / num_of_clusters;
    printf("interval %d \n",interval);
    #pragma omp  parallel for  schedule (dynamic, 1) 
    for(int i = 0; i < imageSize; i = i+interval){
        #pragma opm critical
        centroids[counter] = image_in[i*4];       //R
        #pragma opm critical
        centroids[counter + 1] = image_in[i*4+1]; //G
        #pragma opm critical
        centroids[counter + 2] = image_in[i*4+2]; //B
        #pragma opm critical
        counter +=4;
    }
   
}

void applyNewCentroidValue(int centroidIndex, int* centroids, int* closest_centroid_indices,int *farrest_centroids, unsigned char *image, int size){
    int count = 0;
    int red = 0;
    int green = 0;
    int blue = 0;

    //go through all points and compute average for all belonging to this cluster
    for(int i = 0; i < (size); i++){
        if(closest_centroid_indices[i] == centroidIndex){
            int strindex = i*4;
            #pragma opm critical
            count++;
            red = red + image[strindex];
            green = green + image[strindex+ 1];
            blue = blue + image[strindex + 2];
        }
    }
    // random value to empty cenotroid
    
    if(count == 0){
       
        int randomPixel = (rand() % size) * 4;
        centroids[centroidIndex * 4] = image[randomPixel];
        centroids[centroidIndex * 4 + 1] = image[randomPixel];
        centroids[centroidIndex * 4 + 2] = image[randomPixel];
        centroids[centroidIndex * 4 + 3] = image[centroidIndex * 4 + 3];
    //}
        
       // printf("Warning! Cluster %d has no points. \n", centroidIndex);
    }else {
       // printf("dodelim vrednost %d \n",count);
        centroids[centroidIndex * 4] = red / count;
        centroids[centroidIndex * 4 + 1] = green / count;
        centroids[centroidIndex * 4 + 2] = blue / count;
        centroids[centroidIndex * 4 + 3] = image[centroidIndex * 4 + 3];
    }
}

void applyNewColoursToImage(unsigned char* image, int* closest_centroid_indices,  int size, int* centroids){
    //for each pixel in image assign it new centroid colour   
    #pragma omp  parallel for  schedule (dynamic, 1) 
    for(int i = 0; i < (size); i=i+4){
        //find colour centroid for this pixel
       // printf("index %d \n",closest_centroid_indices[(int)i/3]);
        #pragma opm critical
        int closestCentroid = closest_centroid_indices[(int)i/4];
        //printf("closestcentroid %d\n",closestCentroid);
        //apply centroid colour to this pixel
        if (closestCentroid >=0 && closestCentroid < num_of_clusters){
           /* printf("SLika ima na %d mestu vrednost %d \n",i,centroids[closestCentroid*3]);
            printf("SLika ima na %d mestu vrednost %d \n",i+1,centroids[closestCentroid*3+2]);
            printf("SLika ima na %d mestu vrednost %d \n",i+2,centroids[closestCentroid*3+2]);*/
             #pragma opm critical
            image[i] = centroids[closestCentroid * 4];          //R
             #pragma opm critical
            image[i+1] = centroids[closestCentroid * 4 + 1];    //G
             #pragma opm critical
            image[i+2] = centroids[closestCentroid * 4 + 2];    //B*/
            image[i+3] =255;
       } 
    }
}
int findClosestCentroid(int *centroids, int num_of_clusters, int red, int green, int blue){
    //we init index and distance to 1st centroid, than compute for the remaining ones
    int centroidIndex = 0;
    float minimum_distance = razdalja((float)(centroids[0] - red), (float)(centroids[1] - green),(float)(centroids[2] - blue));

    for(int i = 4; i < (num_of_clusters * 4); i = i + 4){
        float current_distance =  razdalja((float)(centroids[i] - red), (float)(centroids[i+1] - green), (float)(centroids[i+2] - blue));

        if(current_distance < minimum_distance){
            centroidIndex = i / 4;
            minimum_distance = current_distance;
        }
    }
    //printf("najbljižji %d \n",centroidIndex);
    return centroidIndex;
}




int main(int argc, char *argv[])
{
    // preverjanje argumentov če so podani
    if(argv[1]){
       NUM_THREADS=(int)atoi(argv[1]);
    }
    else{
        NUM_THREADS=16;
    }
    if((int)atoi(argv[2])){
        num_of_clusters=(int)atoi(argv[2]);
    }
    else{
        num_of_clusters=64;
    }
   
    omp_set_num_threads(NUM_THREADS);
    printf("stevilo niti %d \n",NUM_THREADS);
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

    double begin = omp_get_wtime();
    initCentroids(centroids,num_of_clusters,image_in,width*height);
    
    for(int I =0; I<interacije;I++){
        // najmanjska evklidova razdalja
        #pragma omp  parallel for schedule (dynamic, 1) 
         for(int point = 0; point < (width*height)*2; point++){
            int imageStartingPointIndex = point * 4;
            int red = image_in[imageStartingPointIndex];
            int green = image_in[imageStartingPointIndex + 1];
            int blue = image_in[imageStartingPointIndex + 2];
            #pragma opm critical
            closest_centroid_indices[point] = findClosestCentroid(centroids, num_of_clusters, red, green, blue);
           
        }
        
        //step 2: for each centroid compute average which will be new centroid
        #pragma omp  parallel for  schedule (dynamic, 1) 
        for(int centroid = 0; centroid < (num_of_clusters); centroid++){
           applyNewCentroidValue(centroid, centroids, closest_centroid_indices,farrest_centroid_indices, image_in, width*height);
        }
        
    } 
    printf("dolocanje novih barv \n");
    applyNewColoursToImage(image_out, closest_centroid_indices,  pitch*height, centroids);
     double end = omp_get_wtime();
    printf("konec programa ostane še pisanje \n");
    //Free source image data
    FreeImage_Unload(imageBitmap32);
    FreeImage_Unload(imageBitmap);
    FIBITMAP *dst = FreeImage_ConvertFromRawBits(image_out, width, height, pitch,
		32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, TRUE);
    FreeImage_Save(FIF_PNG, dst, "output_paralel.png", 0);
    //Free assigned memory 

    double start = omp_get_wtime();
    //openmp();
    double end_paralel = omp_get_wtime();
   printf("Porabljen cas paralelno %f, pri velikosti slike š: %d, v: %d",end-begin,width,height);
    free(image_in);
    free(centroids);
    free(farrest_centroid_indices);
    free(closest_centroid_indices);
    return 0;
}

