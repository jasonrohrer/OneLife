#ifndef EDITOR_CATEGORY_PAGE_INCLUDED
#define EDITOR_CATEGORY_PAGE_INCLUDED




#include "minorGems/ui/event/ActionListener.h"
#include "minorGems/util/SimpleVector.h"

#include "minorGems/game/game.h"


#include "GamePage.h"
#include "TextButton.h"
#include "TextField.h"

#include "Picker.h"

#include "categoryBank.h"
#include "objectBank.h"

#include "keyLegend.h"


#define NUM_TREE_TRANS_TO_SHOW 1



class EditorCategoryPage : public GamePage, public ActionListener {
        
    public:
        EditorCategoryPage();
        ~EditorCategoryPage();

        void clearUseOfObject( int inObjectID );


        virtual void actionPerformed( GUIComponent *inTarget );
        
        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );
        
        virtual void step();
  
        virtual void makeActive( char inFresh );
        

        virtual void pointerMove( float inX, float inY );
        virtual void pointerDown( float inX, float inY );
        virtual void pointerDrag( float inX, float inY );
        virtual void pointerUp( float inX, float inY );
        
        virtual void keyDown( unsigned char inASCII );
        virtual void specialKeyDown( int inKeyCode );
        
    protected:
        
        Picker mObjectParentPicker;
        Picker mObjectChildPicker;

        TextButton mTransEditorButton;

        int mCurrentObject;
        
        // if no object selected, we can pick a category and view its members
        int mCurrentCategory;
        

        int mSelectionIndex;
        
        KeyLegend mKeyLegend;
    };



#endif
