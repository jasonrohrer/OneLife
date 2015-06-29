#ifndef TEXT_BUTTON_INCLUDED
#define TEXT_BUTTON_INCLUDED


#include "Button.h"


#include "minorGems/game/Font.h"


class TextButton : public Button {
        
    public:
        
        // centered on inX, inY
        // label text copied internally
        TextButton( Font *inDisplayFont, 
                    double inX, double inY,
                    const char *inLabelText );

        virtual ~TextButton();
        
        // copied internally
        void setLabelText( const char *inLabelText );
        

    protected:
        Font *mFont;
        char *mLabelText;
    
        // override
        virtual void drawContents();

    };


#endif
