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
          mFillColor( 0.8, 0.8, 0, 1 ),
          mBackFillColor( 0, 0, 0, 1 ),
          mFont( inDisplayFont ),
          mValueField( inDisplayFont, 0, 0, 5,
                       false,
                       inLabelText,
                       ".0123456789-" ),
          mLowValue( inLowValue ),
          mHighValue( inHighValue ),
          mValue( inLowValue ),
          mBarBorder( inBorder ),
          mPointerDown( false ),
          mForceDecimalDigits( false ),
          mDecimalDigits( 0 ) {

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



void ValueSlider::setHighValue( double inHighValue ) {
    mHighValue = inHighValue;
    if( mValue > mHighValue ) {
        mValue = mHighValue;
        }
    setFieldFromValue();
    }



double ValueSlider::getHighValue() {
    return mHighValue;
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



char ValueSlider::isPointerDown() {
    return mPointerDown;
    }



void ValueSlider::setFieldFromValue() {
    int numD = 3;
    
    if( mLowValue <= -100 || mHighValue >= 100 ) {
        numD = 1;
        }
    else if( mLowValue <= -10 || mHighValue >= 10 ) {
        numD = 2;
        }
    
    if( mForceDecimalDigits ) {
        numD = mDecimalDigits;
        }

    mValueField.setFloat( mValue, numD );
    }



void ValueSlider::setFillColor( Color inColor ) {
    mFillColor = inColor;
    }



void ValueSlider::setBackFillColor( Color inColor ) {
    mBackFillColor = inColor;
    }


void ValueSlider::toggleField( char inFieldVisible ) {
    mValueField.setVisible( inFieldVisible );
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

    setDrawColor( mBackFillColor.r, mBackFillColor.g, mBackFillColor.b, 1 );
    
    drawRect( mBarStartX + mBarBorder, mBarStartY + mBarBorder, 
              mBarEndX - mBarBorder, mBarEndY - mBarBorder );

    
    double fillWidth = ( ( mValue - mLowValue ) / ( mHighValue - mLowValue ) )
        * ( mBarEndX - mBarStartX - 2 * mBarBorder );
    
    setDrawColor( mFillColor.r, mFillColor.g, mFillColor.b, mFillColor.a );
    
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
    if( isInBar( inX, inY ) ) {
        mPointerDown = true;
        }
    
    pointerDrag( inX, inY );
    }



void ValueSlider::pointerDrag( float inX, float inY ) {
    if( ! mPointerDown ) {
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
    char wasDown = mPointerDown;
    
    pointerDrag( inX, inY );
    
    mPointerDown = false;

    if( wasDown ) {
        // always fire action on release, even if release is off bar
        fireActionPerformed( this );
        }
    }


 
void ValueSlider::forceDecimalDigits( int inNumDigitsAfterDecimal ) {
    mForceDecimalDigits = true;
    mDecimalDigits = inNumDigitsAfterDecimal;
    }

