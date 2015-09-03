#ifndef SPRITE_TOGGLE_BUTTON_INCLUDED
#define SPRITE_TOGGLE_BUTTON_INCLUDED


#include "SpriteButton.h"


// button that toggles between two sprites (and states) when clicked
class SpriteToggleButton : public SpriteButton {
        
    public:

        SpriteToggleButton( const char *inTGAFileName,
                            const char *inTGAFileNameB,
                            double inX, double inY,
                            double inDrawScale = 1.0 );

        virtual ~SpriteToggleButton();


        char getToggled() {
            return mToggled;
            }

        // does not fire an event
        void setToggled( char inToggled );
        
        
        // set separate tip message for B state
        virtual void setMouseOverTipB( const char *inTipMessageB );

        // overrides from Button to toggle state
        virtual void pointerUp( float inX, float inY ); 

    protected:
        

        SpriteHandle mSpriteB;
        
        char mToggled;
        char mShouldDestroySpriteB;

        char *mMouseOverTipB;
    };


#endif
