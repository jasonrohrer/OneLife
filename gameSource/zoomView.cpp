#include "zoomView.h"


#include "minorGems/game/gameGraphics.h"


void drawZoomView( doublePair inZoomLocation, int inZoomRadius,
                   int inBlowupFactor, doublePair inDrawLocation ) {


    int diam = inZoomRadius * 2;

    Image *im = getScreenRegion( inZoomLocation.x - inZoomRadius, 
                                 inZoomLocation.y - inZoomRadius, 
                                 diam, diam );
    
    
    SpriteHandle zoomSprite = fillSprite( im, false );
    
    delete im;

    if( zoomSprite != NULL ) {
        char oldFilter = getLinearMagFilterOn();
        
        toggleLinearMagFilter( false );
        
        setDrawColor( 1, 1, 1, 1 );
        
        drawSprite( zoomSprite, inDrawLocation, inBlowupFactor );

        freeSprite( zoomSprite );
        
        toggleLinearMagFilter( oldFilter );
        }

    }

