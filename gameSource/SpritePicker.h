

#include "PageComponent.h"
#include "TextButton.h"
#include "TextField.h"


#include "spriteBank.h"

#include "minorGems/game/Font.h"
#include "minorGems/ui/event/ActionListener.h"



class SpritePicker : public PageComponent, ActionListener {
    public:
        
        // centered on inX, inY
        SpritePicker( double inX, double inY );
        
        ~SpritePicker();


        // null if none picked
        SpriteRecord *getSelectedSprite();
        
        
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
        

    };

