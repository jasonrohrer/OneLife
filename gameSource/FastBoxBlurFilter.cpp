#include "FastBoxBlurFilter.h"


#include <string.h>


FastBoxBlurFilter::FastBoxBlurFilter() {	
	
	}





void FastBoxBlurFilter::applySubRegion( unsigned char *inChannel, 
                                        int *inTouchPixelIndices,
                                        int inNumTouchPixels,
                                        int inWidth, int inHeight ) {

    


    // use pointer tricks to walk through neighbor box of each pixel

    // four "corners" around box in accumulation table used to compute
    // box total
    // these are offsets to current accumulation pointer
    int cornerOffsetA = inWidth + 1;
    int cornerOffsetB = -inWidth + 1;
    int cornerOffsetC = inWidth - 1;
    int cornerOffsetD = -inWidth - 1;

    // sides around box
    int sideOffsetA = inWidth;
    int sideOffsetB = -inWidth;
    int sideOffsetC = 1;
    int sideOffsetD = -1;

    unsigned char *sourceData = new unsigned char[ inWidth * inHeight ];
    
    memcpy( sourceData, inChannel, inWidth * inHeight );
    
    
    
    // sum boxes right into passed-in channel

    for( int i=0; i<inNumTouchPixels; i++ ) {

        int pixelIndex = inTouchPixelIndices[ i ];
        

        unsigned char *sourcePointer = &( sourceData[ pixelIndex ] );

        inChannel[ pixelIndex ] =
            ( sourcePointer[ 0 ] +
              sourcePointer[ cornerOffsetA ] +
              sourcePointer[ cornerOffsetB ] +
              sourcePointer[ cornerOffsetC ] +
              sourcePointer[ cornerOffsetD ] +
              sourcePointer[ sideOffsetA ] +
              sourcePointer[ sideOffsetB ] +
              sourcePointer[ sideOffsetC ] +
              sourcePointer[ sideOffsetD ]
              ) / 9;
        }

    delete [] sourceData;
    

    return;
    }
