#ifndef TEXT_AREA_INCLUDED
#define TEXT_AREA_INCLUDED


#include "TextField.h"


// fires action performed when ENTER hit inside area
// (can be toggled to fire on every text change or every focus loss)
class TextArea : public TextField {
        
    public:
        
        // label text and char maps copied internally
        TextArea( Font *inDisplayFont, 
                  double inX, double inY, double inWide, double inHigh,
                  char inForceCaps = false,
                  const char *inLabelText = NULL,
                  const char *inAllowedChars = NULL,
                  const char *inForbiddenChars = NULL );



        virtual void draw();
        
        virtual void specialKeyDown( int inKeyCode );
        virtual void specialKeyUp( int inKeyCode );
    };




#endif
