#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

void usage() {
    printf( "Usage:\n" );
    printf( "audioNoClipTest inA.aiff .... out.aiff\n\n" );


    exit( 1 );
    }




#include "minorGems/sound/formats/aiff.h"
#include "minorGems/sound/audioNoClip.h"
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





int main( int inNumArgs, char **inArgs ) {
    if( inNumArgs < 3 ) {
        usage();
        }


    char *fileNameOut = inArgs[inNumArgs - 1];
    File fileOut( NULL, fileNameOut );
    

    int numFiles = inNumArgs - 2;
    
    int *numSamples = new int[ numFiles ];
    int16_t **samples = new int16_t*[ numFiles ];
    
    
    int longest = 0;

    for( int i=0; i<numFiles; i++ ) {
        


        char *fileName = inArgs[i+1];
    
        File file( NULL, fileName );
        
        samples[i] = readAIFFFile( &file, &( numSamples[i] ) );
    
        if( samples == NULL ) {
            usage();
            }

        if( numSamples[i] > longest ) {
            longest = numSamples[i];
            }
        }
    
    int numA = longest;
            
    double *aFloats = new double[ longest ];
    
    for( int j=0; j<numA; j++ ) {
        aFloats[j] = 0;
        
        }
    
    for( int i=0; i<numFiles; i++ ) {

        for( int j=0; j<numSamples[i]; j++ ) {
            aFloats[j] += (double) samples[i][j] / 32768.0;
            }
        delete [] samples[i];
        }

    delete [] numSamples;
    delete [] samples;
    

    double cap = 0.5;

    NoClip c = resetAudioNoClip( cap, 22050, 22050 );
    
    // dummy right channel
    double *bFloats = new double[ numA ];
    
    memcpy( bFloats, aFloats, numA * sizeof( double ) );
    
    audioNoClip( &c, aFloats, bFloats, numA );

    delete [] bFloats;


    double normalizeFactor = 1.0 / cap;
    
    for( int i=0; i<numA; i++ ) {
        aFloats[i] *= normalizeFactor;
        }


    int16_t *samplesA = new int16_t[ numA ];
    
    for( int i=0; i<numA; i++ ) {
        samplesA[i] = 
            (int16_t)( 
                lrint( 32767 * aFloats[i] ) );
        }


    delete [] aFloats;
            

    writeAiffFile( &fileOut, 44100, samplesA, numA );

    delete [] samplesA;
    

    return 1;
    }
