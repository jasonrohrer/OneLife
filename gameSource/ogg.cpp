
#include "ogg.h"

#include "stb_vorbis.c"


OGGHandle openOGG( File *inOggFile ) {
    char *fileName = inOggFile->getFullFileName();
    
    int error;
    
    stb_vorbis *v = stb_vorbis_open_filename( fileName, &error, NULL );


    delete [] fileName;
    
    // can be NULL, but casting it is fine
    return (void *)v;
    }


int readNextSamplesOGG( OGGHandle inOGG,
                        int inNumSamples, 
                        float *inLeftBuffer,
                        float *inRightBuffer ) {
    stb_vorbis *v = (stb_vorbis*)inOGG;

    float *buffers[2] = { inLeftBuffer, inRightBuffer };
    

    return stb_vorbis_get_samples_float( v, 2, buffers, inNumSamples );
    }


char seekOGG( OGGHandle inOGG, int inNextSample ) {
    stb_vorbis *v = (stb_vorbis*)inOGG;
    
    // returns 0 or 1
    return stb_vorbis_seek( v, inNextSample );
    }





void closeOGG( OGGHandle inOGG ) {
    stb_vorbis *v = (stb_vorbis*)inOGG;
    
    stb_vorbis_close( v );
    }

