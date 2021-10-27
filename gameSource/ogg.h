#include "minorGems/io/file/File.h"

#include <stdint.h>


typedef void* OGGHandle;



// returns NULL on failure

// opens OGG file
OGGHandle openOGG( File *inOggFile );
// opens OGG data from a memory buffer
OGGHandle openOGG( unsigned char *inAllBytes, int inLength );


int getOGGChannels( OGGHandle inOGG );


int getOGGTotalSamples( OGGHandle inOGG );



// returns the number of samples read
int readNextSamplesOGG( OGGHandle inOGG,
                        int inNumSamples, 
                        float *inLeftBuffer,
                        float *inRightBuffer );


void readAllMonoSamplesOGG( OGGHandle inOGG,
                            int16_t *inMonoBuffer );


// seeks in the OGG 
char seekOGG( OGGHandle inOGG, int inNextSample );




void closeOGG( OGGHandle inOGG );



