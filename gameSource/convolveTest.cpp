#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

void usage() {
    printf( "Usage:\n" );
    printf( "convolveTest inA.aiff inB.aiff out.aiff\n\n" );


    exit( 1 );
    }




#include "minorGems/sound/formats/aiff.h"
#include "minorGems/io/file/File.h"
#include "minorGems/io/file/FileOutputStream.h"


static void writeAiffFile( File *inFile, int inSampleRate, int16_t *inSamples, 
                           int inNumSamples ) {
    int headerLength;
    unsigned char *header = 
        getAIFFHeader( 1, 16, inSampleRate, inNumSamples, &headerLength );
        
    FileOutputStream stream( inFile );
    
    stream.write( header, headerLength );

    delete [] header;
        
    int numBytes = inNumSamples * 2;
    unsigned char *data = new unsigned char[ numBytes ];
        
    int b = 0;
    for( int s=0; s<inNumSamples; s++ ) {
        // big endian
        data[b] = inSamples[s] >> 8;
        data[b+1] = inSamples[s] & 0xFF;
        b+=2;
        }
        
    stream.write( data, numBytes );
    delete [] data;
    }




static int16_t *readAIFFFile( File *inFile, int *outNumSamples ) {
    int numBytes;
    unsigned char *data = 
        inFile->readFileContents( &numBytes );
            
    if( data != NULL ) { 
        int16_t *samples = readMono16AIFFData( data, numBytes, outNumSamples );
        
        delete [] data;
        
        return samples;
        }
    
    return NULL;
    }




#include "convolution.h"


int main( int inNumArgs, char **inArgs ) {
    if( inNumArgs != 4 ) {
        usage();
        }
    

    char *fileNameA = inArgs[1];
    char *fileNameB = inArgs[2];
    char *fileNameOut = inArgs[3];
    
    
    File fileA( NULL, fileNameA );
    File fileB( NULL, fileNameB );
    File fileOut( NULL, fileNameOut );
    

    int numA;
    int numB;
    
    int16_t *samplesA = readAIFFFile( &fileA, &numA );
    
    if( samplesA == NULL ) {
        usage();
        }
    
    int16_t *samplesB = readAIFFFile( &fileB, &numB );
    
    if( samplesB == NULL ) {
        usage();
        }
    


    int numWetSamples = numA + numB;
            
    double *wetSampleFloats = new double[ numWetSamples ];
    
    for( int i=0; i<numWetSamples; i++ ) {
        wetSampleFloats[i] = 0;
        }
            
    double *aFloats = new double[ numA ];
    
    for( int j=0; j<numA; j++ ) {
        aFloats[j] = (double) samplesA[j] / 32768.0;
                }

    double *bFloats = new double[ numB ];
            
    for( int i=0; i<numB; i++ ) {
        bFloats[i] = (double) samplesB[i] / 32768.0;
        }

            
    delete [] samplesA;
    delete [] samplesB;


    convolve( aFloats, numA,
              bFloats, numB,
              wetSampleFloats );

    delete [] aFloats;
    delete [] bFloats;

    double maxWet = 0;
    double minWet = 0;
            
    for( int i=0; i<numWetSamples; i++ ) {
        if( wetSampleFloats[ i ] > maxWet ) {
            maxWet = wetSampleFloats[ i ];
            }
        else if( wetSampleFloats[ i ] < minWet ) {
            minWet = wetSampleFloats[ i ];
            }
        }
    double scale = maxWet;
    if( -minWet > scale ) {
        scale = -minWet;
        }
    double normalizeFactor = 1.0 / scale;
    
    int16_t *wetSamples = new int16_t[ numWetSamples ];
    for( int i=0; i<numWetSamples; i++ ) {
        wetSamples[i] = 
            (int16_t)( 
                lrint( 32767 * normalizeFactor * wetSampleFloats[i] ) );
        }
    delete [] wetSampleFloats;
            

    writeAiffFile( &fileOut, 44100, wetSamples, numWetSamples );
    
    delete [] wetSamples;
    

    return 1;
    }
