#ifndef EDITOR_SPRITE_TRIM_PAGE_INCLUDED
#define EDITOR_SPRITE_TRIM_PAGE_INCLUDED


#include "minorGems/ui/event/ActionListener.h"


#include "GamePage.h"

#include "Picker.h"
#include "TextButton.h"


typedef struct PickedRect {
        int xStart;
        int yStart;
        int xEnd; 
        int yEnd;
    } PickedRect;

    

class EditorSpriteTrimPage : public GamePage, public ActionListener {
        
    public:
        EditorSpriteTrimPage();
        ~EditorSpriteTrimPage();
    
        virtual void actionPerformed( GUIComponent *inTarget );


        virtual void drawUnderComponents( doublePair inViewCenter, 
                                          double inViewSize );
        
        virtual void makeActive( char inFresh );
        
        
        virtual void pointerMove( float inX, float inY );
        virtual void pointerDown( float inX, float inY );
        virtual void pointerDrag( float inX, float inY );
        virtual void pointerUp( float inX, float inY );


    protected:
        
        char isPointInSprite( int inX, int inY );
        
        // trims rect so that it doesn't intersect with mRects
        // returns true if inRect is totally eliminated by this process
        char trimRectByExisting( PickedRect *inRect );


        SimpleVector<PickedRect> mRects;

        TextButton mImportEditorButton;
        
        TextButton mSaveButton;

        TextButton mClearRectButton;

        Picker mSpritePicker;
        
        int mPickedSprite;
        

        char mPickingRect;
        
        int mPickStartX, mPickStartY;
        int mPickEndX, mPickEndY;
        
    };

        


#endif
