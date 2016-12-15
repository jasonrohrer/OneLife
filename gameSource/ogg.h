#include "minorGems/io/file/File.h"

#include <stdint.h>


typedef void* OGGHandle;



// returns NULL on failure
OGGHandle openOGG( File *inOggFile );


int getOGGTotalSamples( OGGHandle inOGG );



// returns the number of samples read
int readNextSamplesOGG( OGGHandle inOGG,
                        int inNumSamples, 
                        float *inLeftBuffer,
                        float *inRightBuffer );

// seeks in the OGG 
char seekOGG( OGGHandle inOGG, int inNextSample );




void closeOGG( OGGHandle inOGG );



