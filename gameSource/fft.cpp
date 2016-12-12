
// using fft split radix code from here:
// http://www.kurims.kyoto-u.ac.jp/~ooura/fft.html



#include "fftsg_h.cpp"

#include <string.h>



void realFFT( int inLength, double *inRealInput, double *outFFTValues ) {

    memcpy( outFFTValues, inRealInput, inLength * sizeof( double ) );
    
    rdft( inLength, 1, outFFTValues );
    }



void realInverseFFT( int inLength, double *inFFTValues,
                     double *outRealOutput ) {
    
    memcpy( outRealOutput, inFFTValues, inLength * sizeof( double ) );
    
    rdft( inLength, -1, outRealOutput );
    

    double factor = 2.0 / inLength;
    
    for( int j=0; j<inLength; j++ ) {
        outRealOutput[j] *= factor;
        }
    }

