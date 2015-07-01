#ifndef EDITOR_IMPORT_PAGE_INCLUDED
#define EDITOR_IMPORT_PAGE_INCLUDED




#include "minorGems/ui/event/ActionListener.h"
#include "minorGems/util/SimpleVector.h"

#include "minorGems/game/game.h"


#include "GamePage.h"
#include "TextButton.h"
#include "TextField.h"

#include "SpritePicker.h"



class EditorImportPage : public GamePage, public ActionListener {
        
    public:
        EditorImportPage();
        ~EditorImportPage();
        
        virtual void actionPerformed( GUIComponent *inTarget );
        
        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );
        
        virtual void step();
  
        virtual void makeActive( char inFresh );
        

        virtual void pointerMove( float inX, float inY );
        virtual void pointerDown( float inX, float inY );
        virtual void pointerDrag( float inX, float inY );
        virtual void pointerUp( float inX, float inY );

    protected:
        

        void processSelection();
        

        TextButton mImportButton;

        int mSheetW, mSheetH;

        char mSelect;
        doublePair mSelectStart;
        doublePair mSelectEnd;
        
        Image *mImportedSheet;
        SpriteHandle mImportedSheetSprite;
        

        Image *mProcessedSelection;
        SpriteHandle mProcessedSelectionSprite;

        TextField mSpriteTagField;
        
        TextButton mSaveSpriteButton;

        SpritePicker mSpritePicker;
        
    };



#endif
