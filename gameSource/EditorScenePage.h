#ifndef EDITOR_SCENE_PAGE_INCLUDED
#define EDITOR_SCENE_PAGE_INCLUDED


#include "minorGems/ui/event/ActionListener.h"


#include "GamePage.h"

#include "Picker.h"
#include "TextButton.h"


typedef struct PickedRect {
        int xStart;
        int yStart;
        int xEnd; 
        int yEnd;

        
        // true if there's an intersection on this side
        // order:  up, right, down, left
        char intersectSides[4];
        
        // true if this side is covered by another tile
        // we need to reduce the alpha values of semi-transparent areas
        char coveredSides[4];

    } PickedRect;

    

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
