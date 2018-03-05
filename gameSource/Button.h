#ifndef BUTTON_INCLUDED
#define BUTTON_INCLUDED


#include "PageComponent.h"

#include "minorGems/game/Font.h"
#include "minorGems/ui/event/ActionListenerList.h"


// button superclass that draws a 1-pixel border and handles events
// fires actionPerformed whenever button pressed
class Button : public PageComponent, public ActionListenerList {
        
    public:
        
        // centered on inX, inY
        Button( double inX, double inY,
                double inWide, double inHigh,
                double inPixelSize );

        virtual ~Button();

        virtual void setPixelSize( double inPixelSize );

        // set the tool tip that will be passed up the parent chain
        // when this button is moused over
        // NULL disables tip display for this button
        // copied internally
        virtual void setMouseOverTip( const char *inTipMessage );
        
        
        // overrides to clear state when made invisible
        virtual void setVisible( char inIsVible );

        
        virtual double getWidth();
        virtual double getHeight();

        virtual void setSize( double inWide, double inHigh );


        // defaults to
        // ( 0.828, 0.647, 0.212, 1 ) for drag-over
        // ( 0.886, 0.764, 0.475, 1 ) for hover and
        // white for non-hover
        virtual void setDragOverColor( float r, float g, float b, float a );

        virtual void setNoHoverColor( float r, float g, float b, float a );
        
        virtual void setHoverColor( float r, float g, float b, float a );


        // defaults to
        // ( 0.25, 0.25, 0.25, 1 );
        virtual void setFillColor( float r, float g, float b, float a );
     
        // defaults to
        // ( 0.1, 0.1, 0.1, 1 );
        virtual void setDragOverFillColor( 
            float r, float g, float b, float a );

        
        // defaults to
        // ( 0.75, 0.75, 0.75, 1 );
        virtual void setHoverBorderColor(
            float r, float g, float b, float a );
        

        // if we think about the button border as being
        // made of two brackets, like this:  [button]
        // this is the horizontal length that is enclosed
        // in the brackets (above and below the button text), or how
        // far the top and bottom arms of the brackets reach
        // Defaults to -1, which is a complete, unbroken border
        virtual void setBracketCoverLength( double inLength );
        
        
        // deftaults to 0,0
        // sets a shift that is applied before drawing subclass contents
        // (for example, to shift a font baseline)
        virtual void setContentsShift( doublePair inShift );
        
        
        // should button border and fill be drawn?
        // if not, only button contents (from sub class) is drawn
        virtual void setDrawBackground( char inDraw );


        virtual char isMouseOver();
        
        virtual char isMouseDragOver();
        
        
        virtual void setActive( char inActive );
        virtual char isActive();


    protected:

        virtual void clearState();
        
        virtual void step();
        
        virtual void draw();

        virtual void pointerMove( float inX, float inY );
        virtual void pointerDown( float inX, float inY );
        virtual void pointerDrag( float inX, float inY );
        
        // fires action performed to listener list
        virtual void pointerUp( float inX, float inY );        

        char mActive;

        char mHover;
        char mPressStartedHere;
        char mDragOver;

        double mWide, mHigh, mPixWidth;
        
        char *mMouseOverTip;


        char isInside( float inX, float inY );


        // draw the contents of the button
        // should be overridden by subclasses
        // Button class sets the draw color before calling drawContents
        // (default implementation draws nothing)
        virtual void drawContents();
        
        
        // draws the border of the button
        // default is just a rectangle
        // Button class sets the draw color before calling drawBorder
        virtual void drawBorder();
        

        FloatColor mDragOverColor;
        FloatColor mHoverColor;
        FloatColor mNoHoverColor;
        
        FloatColor mFillColor;
        FloatColor mDragOverFillColor;
        
        FloatColor mHoverBorderColor;

        double mBracketCoverLength;


        doublePair mContentsShift;

        char mDrawBackground;
    };



#endif
