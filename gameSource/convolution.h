

// inDest must be of length inLengthA + inLengthB
void convolve( double *inA, int inLengthA,
               double *inB, int inLengthB,
               double *inDest );



// starts a more efficient convolution where we want to convolve
// multiple A's against the same B, where pre-computation for B
// can be done only once
void startMultiConvolution( double *inB, int inLengthB );


void multiConvolve( double *inA, int inLengthA,
                    double *inDest );


// frees pre-computed resources for B
void endMultiConvolution();

