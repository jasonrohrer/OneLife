#include "Button.h"
#include "minorGems/game/drawUtils.h"
#include "minorGems/game/gameGraphics.h"
#include "minorGems/game/game.h"
#include "minorGems/util/stringUtils.h"

#include <math.h>


Button::Button( double inX, double inY,
                double inWide, double inHigh,
                double inPixelSize )
        : PageComponent( inX, inY ),
          mActive( true ), mHover( false ), mDragOver( false ),
          mWide( inWide ), mHigh( inHigh ), mPixWidth( inPixelSize ),
          mMouseOverTip( NULL ),
          mBracketCoverLength( -1.0 ),
          mDrawBackground( true ) {
    
    
    setDragOverColor( 0.828, 0.647, 0.212, 1 ); 
    
    setHoverColor( 0.886, 0.764, 0.475, 1 );
    
    setNoHoverColor( 1, 1, 1, 1 );
    
    setFillColor( 0.25, 0.25, 0.25, 1 );
    setDragOverFillColor( 0.1, 0.1, 0.1, 1 );

    setHoverBorderColor( 0.75, 0.75, 0.75, 1 );

    mContentsShift.x = 0;
    mContentsShift.y = 0;
    }


Button::~Button() {
    if( mMouseOverTip != NULL ) {
        delete [] mMouseOverTip;
        }
    }


void Button::setActive( char inActive ) {
    mActive = inActive;
    }



char Button::isActive() {
    return mActive;
    }



void Button::setPixelSize( double inPixelSize ) {
    mPixWidth = inPixelSize;
    }



void Button::setSize( double inWide, double inHigh ) {
    mWide = inWide;
    mHigh = inHigh;
    }



void Button::setMouseOverTip( const char *inTipMessage ) {
    if( mMouseOverTip != NULL ) {
        delete [] mMouseOverTip;
        }

    if( inTipMessage != NULL ) {
        mMouseOverTip = stringDuplicate( inTipMessage );
        }
    else {
        mMouseOverTip = NULL;
        }
    }




void Button::setVisible( char inIsVible ) {
    PageComponent::setVisible( inIsVible );
        
    if( ! mVisible ) {
        clearState();
        if( mMouseOverTip != NULL ) {
            clearToolTip( mMouseOverTip );
            }
        }
    }



double Button::getWidth() {
    return mWide;
    }


double Button::getHeight() {
    return mHigh;
    }



void Button::clearState() {
    mHover = false;
    mPressStartedHere = false;
    mDragOver = false;
    }


        
void Button::step() {
    }
        

void Button::draw() {

    if( mDrawBackground ) {
        
        if( mHover && ! mDragOver ) {    
            setDrawColor( mHoverBorderColor );
            }
        else if( mDragOver ) {
            setDrawColor( 0.25, 0.25, 0.25, 1 );
            }
        else {
            setDrawColor( 0.5, 0.5, 0.5, 1 );
            }
    
        drawBorder();
    

        if( mDragOver ) {
            setDrawColor( mDragOverFillColor );
            }
        else {
            setDrawColor( mFillColor );
            }
    
        double rectStartX = - mWide / 2 + mPixWidth;
        double rectStartY = - mHigh / 2 + mPixWidth;

        double rectEndX = mWide / 2 - mPixWidth;
        double rectEndY = mHigh / 2 - mPixWidth;
    
        drawRect( rectStartX, rectStartY,
                  rectEndX, rectEndY );
        }
    

    if( mDragOver ) {    
        setDrawColor( mDragOverColor );
        }
    else if( mHover ) {
        setDrawColor( mHoverColor );
        }
    else {
        setDrawColor( mNoHoverColor );
        }


    doublePair oldViewCenter = getViewCenterPosition();

    setViewCenterPosition( oldViewCenter.x - mContentsShift.x, 
                           oldViewCenter.y - mContentsShift.y );
    
    drawContents();
    
    setViewCenterPosition( oldViewCenter.x, oldViewCenter.y );
    

    if( ! mActive ) {
        // dark overlay
        setDrawColor( 0, 0, 0, 0.5 );
        drawRect( - mWide / 2, - mHigh / 2, 
                  mWide / 2, mHigh / 2 );
        }
    }



void Button::drawContents() {
    }


void Button::drawBorder() {
    if( mBracketCoverLength >= 0 ) {
        // one rect on either end
        drawRect( - mWide / 2, 
                  - mHigh / 2, 
                  - mWide / 2 + mBracketCoverLength, 
                  mHigh / 2 );
        
        drawRect( mWide / 2 - mBracketCoverLength, 
                  - mHigh / 2, 
                  mWide / 2, 
                  mHigh / 2 );
        }
    else {
        // one solid rect
        drawRect( - mWide / 2, - mHigh / 2, 
                  mWide / 2, mHigh / 2 );
        }
    
    }



char Button::isInside( float inX, float inY ) {
    return fabs( inX ) < mWide / 2 &&
        fabs( inY ) < mHigh / 2;
    }



void Button::pointerMove( float inX, float inY ) {
    if( isInside( inX, inY ) ) {
        mHover = true;
        if( mMouseOverTip != NULL ) {
            setToolTip( mMouseOverTip );
            }
        }
    else {
        if( mHover ) {
            // just hovered out
            setToolTip( NULL );
            }
        mHover = false;
        }
    }


void Button::pointerDown( float inX, float inY ) {
    if( isInside( inX, inY ) ) {
        mPressStartedHere = true;
        pointerDrag( inX, inY );
        }
    }



void Button::pointerDrag( float inX, float inY ) {
    if( mPressStartedHere && isInside( inX, inY ) ) {
        if( mMouseOverTip != NULL ) {
            setToolTip( mMouseOverTip );
            }
        mDragOver = true;
        }
    else {
        if( mDragOver ) {
            // just dragged out
            setToolTip( NULL );
            }
        mDragOver = false;
        }
    mHover = false;
    }
        


void Button::pointerUp( float inX, float inY ) {
    if( mPressStartedHere && isInside( inX, inY ) ) {
        mHover = true;
        setToolTip( "" );
        fireActionPerformed( this );
        }
    mPressStartedHere = false;
    mDragOver = false;
    }        





void Button::setDragOverColor( float r, float g, float b, float a ) {
    mDragOverColor.r = r;
    mDragOverColor.g = g;
    mDragOverColor.b = b;
    mDragOverColor.a = a;
    }



void Button::setNoHoverColor( float r, float g, float b, float a ) {
    mNoHoverColor.r = r;
    mNoHoverColor.g = g;
    mNoHoverColor.b = b;
    mNoHoverColor.a = a;
    }


        
void Button::setHoverColor( float r, float g, float b, float a ) {
    mHoverColor.r = r;
    mHoverColor.g = g;
    mHoverColor.b = b;
    mHoverColor.a = a;
    }



void Button::setFillColor( float r, float g, float b, float a ) {
    mFillColor.r = r;
    mFillColor.g = g;
    mFillColor.b = b;
    mFillColor.a = a;
    }



void Button::setDragOverFillColor( float r, float g, float b, float a ) {
    mDragOverFillColor.r = r;
    mDragOverFillColor.g = g;
    mDragOverFillColor.b = b;
    mDragOverFillColor.a = a;
    }



void Button::setHoverBorderColor( float r, float g, float b, float a ) {
    mHoverBorderColor.r = r;
    mHoverBorderColor.g = g;
    mHoverBorderColor.b = b;
    mHoverBorderColor.a = a;
    }



void Button::setBracketCoverLength( double inLength ) {
    mBracketCoverLength = inLength;
    }



void Button::setContentsShift( doublePair inShift ) {
    mContentsShift = inShift;
    }



void Button::setDrawBackground( char inDraw ) {
    mDrawBackground = inDraw;
    }



char Button::isMouseOver() {
    return mHover || mDragOver;
    }


char Button::isMouseDragOver() {
    return mDragOver;
    }

