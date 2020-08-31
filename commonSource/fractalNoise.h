#include <stdint.h>


// sets seed for all subsequent calls
// Two 32-bit seeds, for a 64-bit total seed space
// Second seed defaults to 0.  If 0, seed-response is same as old single-seed
// version.
void setXYRandomSeed( uint32_t inSeedA, uint32_t inSeedB = 0 );



// gets raw 0..1 random values with no fractal blending
double getXYRandom( int inX, int inY );



// scaled to 0..1
// BUT can be larger than 1 sometimes
double getXYFractal( int inX, int inY, double inRoughness, double inScale );

