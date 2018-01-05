#include <stdarg.h>
#include <math.h>

#include "fractalNoise.h"


#define XX_PRIME32_1 2654435761U
#define XX_PRIME32_2 2246822519U
#define XX_PRIME32_3 3266489917U
#define XX_PRIME32_4 668265263U
#define XX_PRIME32_5 374761393U


static uint32_t xxSeed = 0U;



void setXYRandomSeed( uint32_t inSeed ) {
    xxSeed = inSeed;
    }



#define XX_ROTATE_LEFT( inValue, inCount ) \
    ( (inValue << inCount) | (inValue >> (32 - inCount) ) )
      

// original XX hash algorithm as found here:
// https://bitbucket.org/runevision/random-numbers-testing/
/*
static uint32_t xxHash( uint32_t inValue ) {
    uint32_t h32 = xxSeed + XX_PRIME32_5;
    h32 += 4U;
    h32 += inValue * XX_PRIME32_3;
    h32 = XX_ROTATE_LEFT( h32, 17 ) * XX_PRIME32_4;
    h32 ^= h32 >> 15;
    h32 *= XX_PRIME32_2;
    h32 ^= h32 >> 13;
    h32 *= XX_PRIME32_3;
    h32 ^= h32 >> 16;
    return h32;
    }

// modified xxHash to take two int arguments
static uint32_t xxHash2D( uint32_t inX, uint32_t inY ) {
    uint32_t h32 = xxSeed + inX + XX_PRIME32_5;
    h32 += 4U;
    h32 += inY * XX_PRIME32_3;
    h32 = XX_ROTATE_LEFT( h32, 17 ) * XX_PRIME32_4;
    h32 ^= h32 >> 15;
    h32 *= XX_PRIME32_2;
    h32 ^= h32 >> 13;
    h32 *= XX_PRIME32_3;
    h32 ^= h32 >> 16;
    return h32;
    }
*/

// tweaked to be faster by removing lines that don't seem to matter
// for procedural content generation
static uint32_t xxTweakedHash2D( uint32_t inX, uint32_t inY ) {
    uint32_t h32 = xxSeed + inX + XX_PRIME32_5;
    //h32 += 4U;
    h32 += inY * XX_PRIME32_3;
    //h32 = XX_ROTATE_LEFT( h32, 17 ) * XX_PRIME32_4;
    //h32 ^= h32 >> 15;
    h32 *= XX_PRIME32_2;
    h32 ^= h32 >> 13;
    h32 *= XX_PRIME32_3;
    h32 ^= h32 >> 16;
    return h32;
    }


static double oneOverIntMax = 1.0 / ( (double)4294967295U );


double getXYRandom( int inX, int inY ) {
    return xxTweakedHash2D( inX, inY ) * oneOverIntMax;
    }


// in 0..uintMax
// interpolated for inX,inY that aren't integers
static double getXYRandomBN( double inX, double inY ) {
    
    int floorX = lrint( floor(inX) );
    int ceilX = floorX + 1;
    int floorY = lrint( floor(inY) );
    int ceilY = floorY + 1;
    

    double cornerA1 = xxTweakedHash2D( floorX, floorY );
    double cornerA2 = xxTweakedHash2D( ceilX, floorY );

    double cornerB1 = xxTweakedHash2D( floorX, ceilY );
    double cornerB2 = xxTweakedHash2D( ceilX, ceilY );


    double xOffset = inX - floorX;
    double yOffset = inY - floorY;
    
    
    double topBlend = cornerA2 * xOffset + (1-xOffset) * cornerA1;
    
    double bottomBlend = cornerB2 * xOffset + (1-xOffset) * cornerB1;
    

    return bottomBlend * yOffset + (1-yOffset) * topBlend;
    }



double getXYFractal( int inX, int inY, double inRoughness, double inScale ) {

    double b = inRoughness;
    double a = 1 - b;

    double sum =
        a * getXYRandomBN( inX / (32 * inScale), inY / (32 * inScale) )
        +
        b * (
            a * getXYRandomBN( inX / (16 * inScale), inY / (16 * inScale) )
            +
            b * (
                a * getXYRandomBN( inX / (8 * inScale), 
                                   inY / (8 * inScale) )
                +
                b * (
                    a * getXYRandomBN( inX / (4 * inScale), 
                                       inY / (4 * inScale) )
                    +
                    b * (
                        a * getXYRandomBN( inX / (2 * inScale), 
                                           inY / (2 * inScale) )
                        +
                        b * (
                            getXYRandomBN( inX / inScale, inY / inScale )
                            ) ) ) ) );
    
    return sum * oneOverIntMax;
    }
