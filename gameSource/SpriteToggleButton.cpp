
#include "SpriteToggleButton.h"

#include "minorGems/util/log/AppLog.h"
#include "minorGems/util/stringUtils.h"


SpriteToggleButton::SpriteToggleButton( const char *inTGAFileName,
                                        const char *inTGAFileNameB,
                                        double inX, double inY,
                                        double inDrawScale)
        : SpriteButton( inTGAFileName, inX, inY, inDrawScale ),
          mToggled( false ),
          mShouldDestroySpriteB( false ),
          mMouseOverTipB( NULL ) {

    
    Image *imageB = readTGAFile( inTGAFileNameB );

    if( imageB != NULL ) {        
        mSpriteB = fillSprite( imageB );
        delete imageB;

        mShouldDestroySpriteB = true;
        }
    else {
        AppLog::errorF( "Failed to read file for SpriteToggleButton: %s",
                        inTGAFileNameB );
        }
    }


SpriteToggleButton::~SpriteToggleButton() {
    if( mShouldDestroySpriteB ) {
        freeSprite( mSpriteB );
        }
    if( mMouseOverTipB != NULL ) {
        delete [] mMouseOverTipB;
        }
    }



void SpriteToggleButton::setMouseOverTipB( const char *inTipMessageB ) {

    if( mToggled ) {
        // set in active button tip instead
        setMouseOverTip( inTipMessageB );
        }
    else {
        // set alternate message for later
        
        if( mMouseOverTipB != NULL ) {
            delete [] mMouseOverTipB;
            }
        
        if( inTipMessageB != NULL ) {
            mMouseOverTipB = stringDuplicate( inTipMessageB );
            }
        else {
            mMouseOverTipB = NULL;
            }
        }
    }



void SpriteToggleButton::pointerUp( float inX, float inY ) {
    if( isInside( inX, inY ) ) {
        setToggled( ! mToggled );
        
        // let superclass fire event
        Button::pointerUp( inX, inY );

        // but keep tool tip displayed because it has changed 
        // (override superclass)
        setToolTip( mMouseOverTip );

        }

    }        



void SpriteToggleButton::setToggled( char inToggled ) {
    char oldToggled = mToggled;
    mToggled = inToggled;
    
    if( oldToggled != mToggled ) {
        
        // swap sprites and their statuses and tips
        SpriteHandle tempSprite = mSpriteB;
        char tempDestroy = mShouldDestroySpriteB;
        char *tempTip = mMouseOverTipB;
        
        mSpriteB = mSprite;
        mShouldDestroySpriteB = mShouldDestroySprite;
        mMouseOverTipB = mMouseOverTip;
        
        mSprite = tempSprite;
        mShouldDestroySprite = tempDestroy;
        mMouseOverTip = tempTip;
        }
    }

