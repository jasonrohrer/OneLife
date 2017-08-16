#ifndef EDITOR_SCENE_PAGE_INCLUDED
#define EDITOR_SCENE_PAGE_INCLUDED


#include "minorGems/ui/event/ActionListener.h"


#include "GamePage.h"

#include "Picker.h"
#include "TextButton.h"


    

class EditorScenePage : public GamePage, public ActionListener {
        
    public:
        EditorScenePage();
        ~EditorScenePage();
    
        virtual void actionPerformed( GUIComponent *inTarget );


        virtual void drawUnderComponents( doublePair inViewCenter, 
                                          double inViewSize );
        
        virtual void makeActive( char inFresh );
        
        //virtual void specialKeyDown( int inKeyCode );
        
        
    protected:
        
        TextButton mAnimEditorButton;
        
        TextButton mSaveNewButton;
        //TextButton mReplaceButton;
        //TextButton mDeleteButton;


        //TextButton mClearCellButton;

        Picker mGroundPicker;
        Picker mObjectPicker;
        
    };

        


#endif
