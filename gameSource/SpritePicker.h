#ifndef SPRITE_PICKER_INCLUDED
#define SPRITE_PICKER_INCLUDED


#include "PageComponent.h"
#include "TextButton.h"
#include "TextField.h"


#include "spriteBank.h"

#include "minorGems/game/Font.h"
#include "minorGems/ui/event/ActionListener.h"
#include "minorGems/ui/event/ActionListenerList.h"



// fires action performed when selected object changes
class SpritePicker : public PageComponent, ActionListener, 
                     public ActionListenerList  {
    public:
        
        // centered on inX, inY
        SpritePicker( double inX, double inY );
        
        ~SpritePicker();


        // -1 if none picked
        int getSelectedSprite();

        void unselectSprite();
        
        
        void redoSearch();


        virtual void actionPerformed( GUIComponent *inTarget );
        
        
    protected:
        virtual void draw();

        virtual void pointerUp( float inX, float inY );


        int mSkip;
        
        SpriteRecord **mResults;
        int mNumResults;

        TextButton mNextButton;
        TextButton mPrevButton;
        
        TextField mSearchField;
        
        int mSelectionIndex;

    };



#endif
