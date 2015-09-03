#ifndef CHECKBOX_BUTTON_INCLUDED
#define CHECKBOX_BUTTON_INCLUDED


#include "SpriteToggleButton.h"


// toggle button that displays check sprite when toggled
//
// graphics/checkMark.tga and graphics/checkMarkOff.tga
// must exist for this button to work
class CheckboxButton : public SpriteToggleButton {
    
    public:
        
        CheckboxButton( double inX, double inY,
                        double inDrawScale = 1.0 );
        
    };


#endif
