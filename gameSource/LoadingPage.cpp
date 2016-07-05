#include "LoadingPage.h"

#include "message.h"
#include "minorGems/game/drawUtils.h"


void LoadingPage::setCurrentPhase( const char *inPhaseName ) {
    mPhaseName = inPhaseName;
    }


        
void LoadingPage::setCurrentProgress( float inProgress ) {
    mProgress = inProgress;
    }

        

void LoadingPage::draw( doublePair inViewCenter, 
                        double inViewSize ) {

    doublePair labelPos = { 0, 0 };

    drawMessage( "LOADING", labelPos, false );

    labelPos.y = -100;
    
    drawMessage( mPhaseName, labelPos, false );


    if( mShowProgress ) {
        
        // border
        setDrawColor( 1, 1, 1, 1 );
        
        drawRect( -100, -220, 
                  100, -200 );
        
        // inner black
        setDrawColor( 0, 0, 0, 1 );
        
        drawRect( -98, -218, 
                  98, -202 );
        
        
        // progress
        setDrawColor( .8, .8, .8, 1 );
        drawRect( -98, -218, 
                  -98 + mProgress * ( 98 * 2 ), -202 );
        }
    
    }
