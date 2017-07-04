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

        
        // true if there's an intersection on this side
        // order:  up, right, down, left
        char intersectSides[4];
        
        // true if this side is covered by another tile
        // we need to reduce the alpha values of semi-transparent areas
        char coveredSides[4];

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
        // 
        // inUpdateCovered true to update covered flags of existing
        char trimRectByExisting( PickedRect *inRect,
                                 char inUpdateCovered );


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
