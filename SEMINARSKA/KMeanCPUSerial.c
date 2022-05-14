#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include "FreeImage.h"
#include </usr/include/CL/cl.h>

#define NUMBER_OF_ITERATIONS 2

//S tem računamo evklidovo razdaljo sqrt((Rg - Rp)^2 + (Gg-Gp)^2 + (Bg-Bp)^2) --- R,G,B = Barva, g = Gruča, p = pixel
float distance(float x1, float x2, float x3)
{
    return sqrt(pow(x1, 2.0) + pow(x2, 2.0) + pow(x3, 2.0));
}

//Inicializiraj začetne vrednosti centroidov, tako da jim prirediš naključen vzorec x
void initCentroids(int *centroids, int num_of_clusters, unsigned char *image_in, int imageSize){
    int counter = 0;
    //Da lahko preverjamo pravilnost rezultatov, bomo cnetroide naredili z intervalom čez sliko
    int interval = imageSize / num_of_clusters;
    for(int i = 0; i < (num_of_clusters * 3); i = i + 3){
        centroids[i] = image_in[counter];       //R
        centroids[i + 1] = image_in[counter+1]; //G
        centroids[i + 2] = image_in[counter+2]; //B
        counter = counter + (interval * 3);
    }
}
int findClosestCentroid(int *centroids, int num_of_clusters, int red, int green, int blue){
    //we init index and distance to 1st centroid, than compute for the remaining ones
    int centroidIndex = 0;
    float minimum_distance = distance((float)(centroids[0] - red), (float)(centroids[1] - green),(float)(centroids[2] - blue));

    for(int i = 3; i < (num_of_clusters * 3); i = i + 3){
        float current_distance = distance((float)(centroids[i] - red), (float)(centroids[i+1] - green), (float)(centroids[i+2] - blue));

        if(current_distance < minimum_distance){
            centroidIndex = i / 3;
            minimum_distance = current_distance;
        }
    }
    return centroidIndex;
}
void applyNewCentroidValue(int centroidIndex, int* centroids, int* closest_centroid_indices, unsigned char *image, int size){
    int count = 0;
    int red = 0;
    int green = 0;
    int blue = 0;

    //go through all points and compute average for all belonging to this cluster
    for(int i = 0; i < (size); i++){
        if(closest_centroid_indices[i] == centroidIndex){
            count++;
            red = red + image[i * 3];
            green = green + image[i * 3 + 1];
            blue = blue + image[i * 3 + 2];
        }
    }
    //compute average,
    //TODO: Izračunaj Prazno gručo
    // Pri razvrščanju se lahko zgodi, da katera izmed gruč ostane prazna. Temu se običajno
    //želimo izogniti. To lahko storimo tako, da prazni gruči pripišemo tisti vzorec, ki je najdlje
    if(count == 0){
        //printf("Warning! Cluster %d has no points. \n", centroidIndex);
    }else {
        centroids[centroidIndex * 3] = red / count;
        centroids[centroidIndex * 3 + 1] = green / count;
        centroids[centroidIndex * 3 + 2] = blue / count;
    }
}

void applyNewColoursToImage(unsigned char* image, int* closest_centroid_indices,  int size, int* centroids){
    //for each pixel in image assign it new centroid colour
    for(int i = 0; i < (size); i = i + 3){
        //find colour centroid for this pixel
        int closestCentroid = closest_centroid_indices[i/3];
        //apply centroid colour to this pixel
        if (closestCentroid < size)
        {
             image[i] = centroids[closestCentroid * 3];          //R
        image[i+1] = centroids[closestCentroid * 3 + 1];    //G
        image[i+2] = centroids[closestCentroid * 3 + 2];    //B

        }
        
       
    }
}

void KMeanCPUSerial(unsigned char *image_in, int width, int height, int num_of_clusters, int pitch)
{
    int *centroids = (int*)malloc(num_of_clusters * 3 * sizeof(int));
    initCentroids(centroids, num_of_clusters, image_in, width * height);

    //init array for keeping indices of closest centroid
    int *closest_centroid_indices = (int*)malloc(width * height * sizeof(int));

    for(int iteration = 0; iteration < (NUMBER_OF_ITERATIONS); iteration++)
    {
        //step 1: go through all points and find closest centroid
        for(int point = 0; point < (width*height); point++){
            int imageStartingPointIndex = point * 3;
            int red = image_in[imageStartingPointIndex];
            int green = image_in[imageStartingPointIndex + 1];
            int blue = image_in[imageStartingPointIndex + 2];
            closest_centroid_indices[point] = findClosestCentroid(centroids, num_of_clusters, red, green, blue);
        }

        //step 2: for each centroid compute average which will be new centroid
        for(int centroid = 0; centroid < (num_of_clusters); centroid++){
            applyNewCentroidValue(centroid, centroids, closest_centroid_indices, image_in, width*height);
        }
    }

    //apply new colours to input image
    applyNewColoursToImage(image_in, closest_centroid_indices, pitch*height, centroids);
}

int main(int argc, char const *argv[])
{
   
    printf("test v main\n");
    FIBITMAP *imageBitmap = FreeImage_Load(FIF_PNG, "brd.png", 0);
    FIBITMAP *imageBitmap8 = FreeImage_ConvertTo32Bits(imageBitmap);
    int width = FreeImage_GetWidth(imageBitmap8);
    int height = FreeImage_GetHeight(imageBitmap8);
    int pitch = FreeImage_GetPitch(imageBitmap8);


    unsigned char *image_in = (unsigned char *)malloc(height * pitch * sizeof(unsigned char));
    FreeImage_ConvertToRawBits(image_in, imageBitmap8, pitch, 32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_GREEN_MASK, TRUE);

    

    KMeanCPUSerial(image_in,width,height,3,pitch);
    FIBITMAP *dst = FreeImage_ConvertFromRawBits(image_in, width, height, pitch,
		32, FI_RGBA_RED_MASK, FI_RGBA_GREEN_MASK, FI_RGBA_BLUE_MASK, TRUE);
    FreeImage_Save(FIF_PNG, dst, "output.png", 0);
    
    return 0;
}
