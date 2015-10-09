#include "ValueSlider.h"

#include "minorGems/util/stringUtils.h"

#include "minorGems/game/drawUtils.h"



ValueSlider::ValueSlider( Font *inDisplayFont, 
                          double inX, double inY,
                          double inBorder,
                          double inWidth, double inHeight,
                          double inLowValue,
                          double inHighValue,
                          const char *inLabelText )
        : PageComponent( inX, inY ),
          mFont( inDisplayFont ),
          mValueField( inDisplayFont, 0, 0, 5,
                       false,
                       inLabelText,
                       ".0123456789-" ),
          mLowValue( inLowValue ),
          mHighValue( inHighValue ),
          mValue( inLowValue ),
          mBarBorder( inBorder ) {

    addComponent( &mValueField );
    mValueField.addActionListener( this );

    mValueField.setFireOnLoseFocus( true );

    mBarStartX = mValueField.getRightEdgeX() + 20;
    
    mBarEndX = mBarStartX + inWidth;

    mBarStartY = - inHeight/2;
    mBarEndY = mBarStartY + inHeight;
    }



ValueSlider::~ValueSlider() {
    }



double ValueSlider::getValue() {
    return mValue;
    }


void ValueSlider::setValue( double inValue ) {
    mValue = inValue;
    if( mValue < mLowValue ) {
        mValue = mLowValue;
        }
    if( mValue > mHighValue ) {
        mValue = mHighValue;
        }
    setFieldFromValue();
    }



void ValueSlider::setFieldFromValue() {
    int numD = 3;
    
    if( mLowValue < -10 || mHighValue > 10 ) {
        numD = 1;
        }
    if( mLowValue < -1 || mHighValue > 1 ) {
        numD = 2;
        }
    
    mValueField.setFloat( mValue, numD );
    }



void ValueSlider::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mValueField ) {
        double value = (double)( mValueField.getFloat() );
        
        double setValue = value;
        
        if( value < mLowValue ) {
            value = mLowValue;
            }
        if( value > mHighValue ) {
            value = mHighValue;
            }
        
        mValue = value;
        
        if( value != setValue ) {
            setFieldFromValue();
            }
        
        fireActionPerformed( this );
        }
    
    }



void ValueSlider::draw() {
    
    setDrawColor( 1, 1, 1, 1 );
    
    drawRect( mBarStartX, mBarStartY, 
              mBarEndX, mBarEndY );

    setDrawColor( 0, 0, 0, 1 );
    
    drawRect( mBarStartX + mBarBorder, mBarStartY + mBarBorder, 
              mBarEndX - mBarBorder, mBarEndY - mBarBorder );

    
    double fillWidth = ( ( mValue - mLowValue ) / ( mHighValue - mLowValue ) )
        * ( mBarEndX - mBarStartX - 2 * mBarBorder );
    
    setDrawColor( .8, .8, 0, 1 );
    
    drawRect( mBarStartX + mBarBorder, mBarStartY + mBarBorder, 
              mBarStartX + mBarBorder + fillWidth, mBarEndY - mBarBorder );
    
    }



char ValueSlider::isInBar( float inX, float inY ) {
    // let mouse move off ends in x direction a bit extra
    if( inX > mBarStartX - 2 * mBarBorder && inX < mBarEndX + 2 * mBarBorder &&
        inY > mBarStartY && inY < mBarEndY ) {
        return true;
        }
    return false;        
    }



void ValueSlider::pointerDown( float inX, float inY ) {
    pointerDrag( inX, inY );
    }



void ValueSlider::pointerDrag( float inX, float inY ) {
    if( ! isInBar( inX, inY ) ) {
        return;
        }
    
    mValue = mLowValue + 
        (mHighValue - mLowValue ) * 
        ( inX - mBarStartX - mBarBorder ) / 
        ( mBarEndX - mBarStartX - 2 * mBarBorder );
    
    if( mValue < mLowValue ) {
        mValue = mLowValue;
        }
    if( mValue > mHighValue ) {
        mValue = mHighValue;
        }
    

    setFieldFromValue();
    fireActionPerformed( this );
    }




void ValueSlider::pointerUp( float inX, float inY ) {
    pointerDrag( inX, inY );
    }


 
