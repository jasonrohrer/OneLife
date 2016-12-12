#include "convolution.h"


#include "minorGems/system/Time.h"

#include <stdio.h>
#include <math.h>
#include <string.h>


static void naiveConvolve( double *inA, int inLengthA,
                           double *inB, int inLengthB,
                           double *inDest ) {
    for( int i=0; i<inLengthA; i++ ) {
        for( int j=0; j<inLengthB; j++ ) {
            inDest[ i + j ] +=
                inA[i] * inB[j];
            }
        }
    }



static double *zeroPad( double *inSource, 
                        int inSourceLength, int inNewLength ) {
    double *padded = new double[ inNewLength ];
    
    for( int i=0; i<inNewLength; i++ ) {
        padded[i] = 0.0;
        }
    memcpy( padded, inSource, sizeof( double ) * inSourceLength );

    return padded;
    }



static void windowConvolve( int inWindowSize,
                            double *inA, int inLengthA,
                            double *inB, int inLengthB,
                            double *inDest ) {
    int windowsA = lrint( ceil( inLengthA / (double)inWindowSize ) );
    int windowsB = lrint( ceil( inLengthB / (double)inWindowSize ) );


    double *paddedA = zeroPad( inA, inLengthA, windowsA * inWindowSize );
    double *paddedB = zeroPad( inB, inLengthB, windowsB * inWindowSize );
    
    double *paddedDest = zeroPad( inDest, inLengthA + inLengthB,
                                  windowsA * inWindowSize +
                                  windowsB * inWindowSize );
    
    for( int a=0; a<windowsA; a++ ) {
        int offsetA = a * inWindowSize;
        for( int b=0; b<windowsB; b++ ) {
            int offsetB = b * inWindowSize;
            
            int destOffset = offsetA + offsetB;
            
            naiveConvolve( &( inA[offsetA] ), inWindowSize,
                           &( inB[offsetB] ), inWindowSize,
                           &( paddedDest[ destOffset ] ) );
            }
        }
    
    memcpy( inDest, paddedDest, sizeof( double ) * ( inLengthA + inLengthB ) );
    
    delete [] paddedA;
    delete [] paddedB;
    delete [] paddedDest;
    }




void convolve( double *inA, int inLengthA,
               double *inB, int inLengthB,
               double *inDest ) {
    
    double start = Time::getCurrentTime();
    
    //    naiveConvolve( inA, inLengthA, inB, inLengthB, inDest );
    windowConvolve( 512, inA, inLengthA, inB, inLengthB, inDest );
    
    printf( "Convolution of %dx%d took %.3f seconds\n",
            inLengthA, inLengthB, Time::getCurrentTime() - start );
    }

