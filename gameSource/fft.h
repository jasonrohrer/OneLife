


// output contains real and imaginary parts interleaved
// the realFFT produces half as many complex values, so inComplexOuput
// has inLength doubles in it
void realFFT( int inLength, double *inRealInput, double *outFFTValues );


// input FFT values in the same interleaved order produced by realFFT
void realInverseFFT( int inLength, double *inFFTValues,
                     double *outRealOutput );
