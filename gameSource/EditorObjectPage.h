#ifndef EDITOR_OBJECT_PAGE_INCLUDED
#define EDITOR_OBJECT_PAGE_INCLUDED




#include "minorGems/ui/event/ActionListener.h"
#include "minorGems/util/SimpleVector.h"

#include "minorGems/game/game.h"


#include "GamePage.h"
#include "TextButton.h"
#include "TextField.h"

#include "Picker.h"

#include "objectBank.h"



class EditorObjectPage : public GamePage, public ActionListener {
        
    public:
        EditorObjectPage();
        ~EditorObjectPage();
        
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

        TextField mDescriptionField;
        
        TextButton mSaveObjectButton;
        TextButton mReplaceObjectButton;

        TextButton mClearObjectButton;

        
        TextButton mImportEditorButton;

        Picker mSpritePicker;
        Picker mObjectPicker;


        ObjectRecord mCurrentObject;

        int mPickedObjectLayer;
        doublePair mPickedMouseOffset;
    };



#endif
