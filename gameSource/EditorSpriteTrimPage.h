#ifndef EDITOR_SPRITE_TRIM_PAGE_INCLUDED
#define EDITOR_SPRITE_TRIM_PAGE_INCLUDED


#include "minorGems/ui/event/ActionListener.h"


#include "GamePage.h"

#include "Picker.h"
#include "TextButton.h"


class EditorSpriteTrimPage : public GamePage, public ActionListener {
        
    public:
        EditorSpriteTrimPage();
        ~EditorSpriteTrimPage();
    
        virtual void actionPerformed( GUIComponent *inTarget );


        virtual void drawUnderComponents( doublePair inViewCenter, 
                                          double inViewSize );
        
        virtual void makeActive( char inFresh );


    protected:
        

        TextButton mImportEditorButton;
        
        TextButton mSaveButton;

        Picker mSpritePicker;
        
        int mPickedSprite;
        
    };

        


#endif
