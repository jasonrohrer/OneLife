#include "convolution.h"


#include "minorGems/system/Time.h"

#include <stdio.h>


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




void convolve( double *inA, int inLengthA,
               double *inB, int inLengthB,
               double *inDest ) {
    
    double start = Time::getCurrentTime();
    
    naiveConvolve( inA, inLengthA, inB, inLengthB, inDest );
    
    printf( "Convolution of %dx%d took %.3f seconds\n",
            inLengthA, inLengthB, Time::getCurrentTime() - start );
    }

