#ifndef KEY_EQUIVALENT_TEXT_BUTTON_INCLUDED
#define KEY_EQUIVALENT_TEXT_BUTTON_INCLUDED


#include "TextButton.h"


class KeyEquivalentTextButton : public TextButton {
        
    public:
        
        
        // inKeyA and inKeyB trigger this button when they are pressed
        // along with the platform-specific command key
        KeyEquivalentTextButton( Font *inDisplayFont, 
                                 double inX, double inY,
                                 const char *inLabelText,
                                 unsigned char inKeyA, unsigned char inKeyB );


  
    protected:
        
        unsigned char mKeyA, mKeyB;

        virtual void keyDown( unsigned char inASCII );
        
    };


#endif
