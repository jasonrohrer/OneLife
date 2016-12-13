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



#include "fft.h"


static void fftConvolve( int inWindowSize,
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
    
    double *fftBufferA = new double[ inWindowSize * 2 ];
    double *fftBufferB = new double[ inWindowSize * 2 ];
    
    double *bufferResult = new double[ inWindowSize * 2 ];

    for( int a=0; a<windowsA; a++ ) {
        int offsetA = a * inWindowSize;

        double *paddedAWindow = zeroPad( &( paddedA[offsetA] ),
                                         inWindowSize, inWindowSize * 2 );
        
        realFFT( inWindowSize * 2, paddedAWindow, fftBufferA );
        

        /*
        // test:  just compute inverse directly
        realInverseFFT( inWindowSize * 2, fftBufferA, bufferResult );

        for( int i=0; i<inWindowSize * 2; i++ ) {
            paddedDest[ offsetA + i ] += bufferResult[i];
            }
        */
        
        for( int b=0; b<windowsB; b++ ) {
            int offsetB = b * inWindowSize;
            
            double *paddedBWindow = zeroPad( &( paddedB[offsetB] ),
                                             inWindowSize, inWindowSize * 2 );
        
            realFFT( inWindowSize * 2, paddedBWindow, fftBufferB );

            int destOffset = offsetA + offsetB;
            
            // need to perform complex multiplication
            // output data in this order:
            //                    a[2*k] = R[k], 0<=k<n/2
            //                    a[2*k+1] = I[k], 0<k<n/2
            //                    a[1] = R[n/2]
            // X[0] and X[n/2] are real valued
            // with n/2 - 1 complex values in between

            // real-only values first
            fftBufferB[0] *= fftBufferA[0];

            fftBufferB[1] *= fftBufferA[1];


            for( int k=1; k<inWindowSize; k++ ) {
                int realIndex = 2 * k;
                int imIndex = realIndex + 1;
                
                double realA = fftBufferA[ realIndex ];
                double realB = fftBufferB[ realIndex ];
                
                double imA = fftBufferA[ imIndex ];
                double imB = fftBufferB[ imIndex ];
                
                double realP = realA * realB - imA * imB;
                double imP = realA * imB + realB * imA;

                fftBufferB[realIndex] = realP;
                fftBufferB[imIndex] = imP;
                }
            
            realInverseFFT( inWindowSize * 2, fftBufferB, bufferResult );

            for( int i=0; i<inWindowSize * 2; i++ ) {
                paddedDest[ destOffset + i ] += bufferResult[i];
                }
            delete [] paddedBWindow;
            }
        
        delete [] paddedAWindow;
        }

    delete [] fftBufferA;
    delete [] fftBufferB;
    delete [] bufferResult;
    
    
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
    //windowConvolve( 512, inA, inLengthA, inB, inLengthB, inDest );
    fftConvolve( 65536, inA, inLengthA, inB, inLengthB, inDest );
    
    printf( "Convolution of %dx%d took %.3f seconds\n",
            inLengthA, inLengthB, Time::getCurrentTime() - start );
    }

