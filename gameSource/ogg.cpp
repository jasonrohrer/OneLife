
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



OGGHandle openOGG( unsigned char *inAllBytes, int inLength ) {

    int error;

    stb_vorbis *v = stb_vorbis_open_memory( inAllBytes, inLength,
                                            &error, NULL );
    // can be NULL, but casting it is fine
    return (void *)v;
    }



int getOGGChannels( OGGHandle inOGG ) {
    stb_vorbis *v = (stb_vorbis*)inOGG;

    return stb_vorbis_get_info( v ).channels;
    }



int getOGGTotalSamples( OGGHandle inOGG ) {
    stb_vorbis *v = (stb_vorbis*)inOGG;

    return (int)( stb_vorbis_stream_length_in_samples( v ) );
    }



int readNextSamplesOGG( OGGHandle inOGG,
                        int inNumSamples, 
                        float *inLeftBuffer,
                        float *inRightBuffer ) {
    stb_vorbis *v = (stb_vorbis*)inOGG;

    float *buffers[2] = { inLeftBuffer, inRightBuffer };
    

    return stb_vorbis_get_samples_float( v, 2, buffers, inNumSamples );
    }



void readAllMonoSamplesOGG( OGGHandle inOGG,
                            int16_t *inMonoBuffer ) {
    stb_vorbis *v = (stb_vorbis*)inOGG;

    int16_t *buffers[1] = { inMonoBuffer };
    

    stb_vorbis_get_samples_short( v, 1, buffers, 
                                  getOGGTotalSamples( inOGG ) );
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

