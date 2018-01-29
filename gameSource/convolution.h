

// inDest must be of length inLengthA + inLengthB
void convolve( double *inA, int inLengthA,
               double *inB, int inLengthB,
               double *inDest );




typedef struct MultiConvolution {
        // set to -1 if not initialized
        int savedNumWindowsB;
        int savedNumSamplesB;
        double **savedFFTBufferB;
        int savedWindowSize;
    } MultiConvolution;
    


// starts a more efficient convolution where we want to convolve
// multiple A's against the same B, where pre-computation for B
// can be done only once
MultiConvolution startMultiConvolution( double *inB, int inLengthB );


void multiConvolve( MultiConvolution inMulti, double *inA, int inLengthA,
                    double *inDest );


// frees pre-computed resources for B
void endMultiConvolution( MultiConvolution *inMulti );

