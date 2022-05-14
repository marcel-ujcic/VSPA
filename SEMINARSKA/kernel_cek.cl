// kernel1 racunanje razdalj
__kernel void calculateDistances(__global unsigned char * image_in,__global unsigned char * image_out,
                       __global unsigned int* Bc, __global unsigned int* Gc, __global unsigned int* Rc,
                       int width, int height,__global int* indexSkupinePixla,
                       __global unsigned int* Bskupine, __global unsigned int* Gskupine, __global unsigned int* Rskupine,
                       __global  int* VelikostSkupine, __global int* Randomint,int Skupine,
                       __local unsigned int * RSkupineloc,__local unsigned int * GSkupineloc,__local unsigned int * BSkupineloc,
                       __local int * indexSkupinePixlaloc,__local int * VelikostSkupineloc)						
{	

       
        int gid = get_global_id(0);
	    int lid = get_local_id(0);

    if(gid<(height*width)){

        float razdalja;
        int indexNajmanjse;
        razdalja = sqrt(pow((double)image_in[gid*4]-(double)Bc[0], 2) + pow((double)image_in[gid*4+1]-(double)Gc[0], 2) + pow((double)image_in[gid*4+2]-(double)Rc[0], 2));
        indexNajmanjse = 0;
        //vsaka skupina
        for(int c = 1; c < Skupine; c++){
            int evk = sqrt(pow((double)image_in[gid*4]-(double)Bc[c], 2) + pow((double)image_in[gid*4+1]-(double)Gc[c], 2) + pow((double)image_in[gid*4+2]-(double)Rc[c], 2));
            if(evk < razdalja){
                razdalja = evk;
                indexNajmanjse = c;
            }
        }

    indexSkupinePixlaloc[gid] = indexNajmanjse;
    atomic_add(&BSkupineloc[indexNajmanjse], image_in[gid*4]);
    atomic_add(&GSkupineloc[indexNajmanjse], image_in[gid*4+1]);
    atomic_add(&RSkupineloc[indexNajmanjse], image_in[gid*4+2]);
    atomic_add(&VelikostSkupineloc[indexNajmanjse], 1);

    barrier(CLK_LOCAL_MEM_FENCE);
    
    indexSkupinePixla[gid] = indexSkupinePixlaloc[gid];
    atomic_add(&Bskupine[lid],BSkupineloc[lid]);
    atomic_add(&Gskupine[lid],GSkupineloc[lid]);
    atomic_add(&Rskupine[lid],RSkupineloc[lid]);
    atomic_add(&VelikostSkupine[lid],VelikostSkupineloc[lid]);
    

    }
}

// kernel racunanje povprecji
__kernel void calculateAverages(__global unsigned char * image_in,__global unsigned char * image_out,
                       __global unsigned int* Bc, __global unsigned int* Gc, __global unsigned int* Rc,
                       int width, int height,__global int* indexSkupinePixla,
                       __global unsigned int* Bskupine, __global unsigned int* Gskupine, __global unsigned int* Rskupine,
                       __global  int* VelikostSkupine, __global int* Randomint,int Skupine)						
{	

    int gid = get_global_id(0);

        if(VelikostSkupine[gid] > 0){
            Bc[gid] = Bskupine[gid] / VelikostSkupine[gid];
            Gc[gid] = Gskupine[gid] / VelikostSkupine[gid];
            Rc[gid] = Rskupine[gid] / VelikostSkupine[gid];
        } else{
            int randomPixel = Randomint[gid];
            Bc[gid] = image_in[randomPixel];
            Gc[gid] = image_in[randomPixel+1];
            Rc[gid] = image_in[randomPixel+2];
        }

        VelikostSkupine[gid] = 0;
        Bskupine[gid] = 0;
        Gskupine[gid] = 0;
        Rskupine[gid] = 0;
        }
//kernel zapis slike
__kernel void writePicture(__global unsigned char * image_in,__global unsigned char * image_out,
                       __global unsigned int* Bc, __global unsigned int* Gc, __global unsigned int* Rc,
                       int width, int height,__global int* indexSkupinePixla,
                       __global unsigned int* Bskupine, __global unsigned int* Gskupine, __global unsigned int* Rskupine,
                       __global  int* VelikostSkupine, __global int* Randomint,int Skupine)						
{	
        int gid = get_global_id(0);
    if(gid<(height*width)){

        image_out[gid*4] =  Bc[indexSkupinePixla[gid]];
        image_out[gid*4+1] = Gc[indexSkupinePixla[gid]];
        image_out[gid*4+2] = Rc[indexSkupinePixla[gid]];
        image_out[gid*4+3] = image_in[gid*4+3];
        }

}