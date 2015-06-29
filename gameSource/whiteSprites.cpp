#include "whiteSprites.h"


SpriteHandle loadWhiteSprite( const char *inTGAFileName,
                              int *outW,
                              int *outH ) {

    Image *spriteImage = readTGAFile( inTGAFileName );

    if( spriteImage == NULL ) {
        return NULL;
        }

    if( outW != NULL ) {
        *outW = spriteImage->getWidth();
        }
    if( outH != NULL ) {
        *outH = spriteImage->getHeight();
        }

    SpriteHandle sprite = fillWhiteSprite( spriteImage );

    delete spriteImage;
    
    return sprite;
    }



SpriteHandle fillWhiteSprite( Image *inImage ) {
        
    
    int width = inImage->getWidth();
        
    int height = inImage->getHeight();
    
    int numPixels = width * height;
    
    
    Image rgbaImage( width, height, 4, false );
    
    
    
    // red into alpha
    memcpy( rgbaImage.getChannel( 3 ), inImage->getChannel( 0 ),
            sizeof( double ) * numPixels );
    
    // white into rest
    double *solidWhite = new double[ numPixels ];
    for( int i=0; i<numPixels; i++ ) {
        solidWhite[i] = 1;
        }
    
    // white into others
    memcpy( rgbaImage.getChannel( 0 ), solidWhite, 
            sizeof( double ) * numPixels ); 

    memcpy( rgbaImage.getChannel( 1 ), solidWhite, 
            sizeof( double ) * numPixels ); 

    memcpy( rgbaImage.getChannel( 2 ), solidWhite, 
            sizeof( double ) * numPixels ); 

    
    delete [] solidWhite;

    
    
    
    return fillSprite( &rgbaImage, false );
    }
