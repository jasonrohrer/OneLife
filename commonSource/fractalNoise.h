#include <stdint.h>


// sets seed for all subsequent calls
void setXYRandomSeed( uint32_t inSeed );



// gets raw 0..1 random values with no fractal blending
double getXYRandom( int inX, int inY );



// scaled to 0..1
// BUT can be larger than 1 sometimes
double getXYFractal( int inX, int inY, double inRoughness, double inScale );

