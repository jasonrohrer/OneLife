#ifndef VALUE_SLIDER_INCLUDED
#define VALUE_SLIDER_INCLUDED


#include "PageComponent.h"
#include "TextField.h"

#include "minorGems/game/Font.h"
#include "minorGems/ui/event/ActionListenerList.h"


class ValueSlider : public PageComponent, public ActionListenerList,
                    public ActionListener {
        
    public:

        ValueSlider( Font *inDisplayFont, 
                     double inX, double inY,
                     // thickness of border
                     double inBorder,
                     // size of slider bar, not including text field
                     double inWidth, double inHeight,
                     double inLowValue,
                     double inHighValue,
                     const char *inLabelText );
        
        ~ValueSlider();
        
        
        double getValue();
        void setValue( double inValue );

        void setHighValue( double inHighValue );
        
        double getHighValue();
        

        char isPointerDown();
        

        void setFillColor( Color inColor );
        void setBackFillColor( Color inColor );

        // toggles whether a text field is visible next to the slider
        // defaults to true
        void toggleField( char inFieldVisible );
        

        // usually automatic based on slider range
        // but can be forced to control precision
        void forceDecimalDigits( int inNumDigitsAfterDecimal );
        

    protected:
        virtual void actionPerformed( GUIComponent *inTarget );

        virtual void draw();

        virtual void pointerDown( float inX, float inY );
        virtual void pointerDrag( float inX, float inY );
        
        // fires action performed to listener list
        virtual void pointerUp( float inX, float inY );        

        char isInBar( float inX, float inY );

        void setFieldFromValue();
        
        
        Color mFillColor;
        Color mBackFillColor;
        
        Font *mFont;
        TextField mValueField;

        double mLowValue, mHighValue;
        double mValue;
        

        double mBarBorder;
        double mBarStartX, mBarEndX, mBarStartY, mBarEndY;

        char mPointerDown;

        char mForceDecimalDigits;
        int mDecimalDigits;
    };


#endif
