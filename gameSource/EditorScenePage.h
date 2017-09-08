#ifndef EDITOR_SCENE_PAGE_INCLUDED
#define EDITOR_SCENE_PAGE_INCLUDED


#include "minorGems/ui/event/ActionListener.h"


#include "GamePage.h"

#include "Picker.h"
#include "TextButton.h"
#include "ValueSlider.h"

#include "objectBank.h"
#include "animationBank.h"

#include "keyLegend.h"

#include "RadioButtonSet.h"



typedef struct SceneCell {
        int biome;
        
        int oID;

        int heldID;
        
        SimpleVector<int> contained;
        SimpleVector< SimpleVector<int> > subContained;

        ClothingSet clothing;

        char flipH;
        double age;
        
        AnimType anim;
        
        // negative to unfreeze time for cell, or positive to freeze
        // time at a given point
        double frozenAnimTime;
        
        // for vanishing/appearing sprites based on use
        int numUsesRemaining;
        

    } SceneCell;


#define SCENE_W 12
#define SCENE_H 7    
    

class EditorScenePage : public GamePage, public ActionListener {
        
    public:
        EditorScenePage();
        ~EditorScenePage();
    
        virtual void actionPerformed( GUIComponent *inTarget );


        virtual void drawUnderComponents( doublePair inViewCenter, 
                                          double inViewSize );
        
        virtual void makeActive( char inFresh );
        
        virtual void keyDown( unsigned char inASCII );
        virtual void specialKeyDown( int inKeyCode );
        
        
    protected:
        
        TextButton mAnimEditorButton;
        
        TextButton mSaveNewButton;
        //TextButton mReplaceButton;
        //TextButton mDeleteButton;


        //TextButton mClearCellButton;

        Picker mGroundPicker;
        Picker mObjectPicker;
        
        ValueSlider mPersonAgeSlider;
        
        RadioButtonSet mCellAnimRadioButtons;
        RadioButtonSet mPersonAnimRadioButtons;
        
        ValueSlider mCellAnimFreezeSlider;
        ValueSlider mPersonAnimFreezeSlider;
        
        ValueSlider mCellSpriteVanishSlider;


        SpriteHandle mGroundOverlaySprite[4];

        SceneCell mCells[SCENE_H][SCENE_W];
        SceneCell mPersonCells[SCENE_H][SCENE_W];
        
        SceneCell mEmptyCell;
        SceneCell mCopyBuffer;
        

        int mCurX, mCurY;
        
        double mFrameCount;
        
        KeyLegend mKeyLegend, mKeyLegendG, mKeyLegendC, mKeyLegendP;


        void floodFill( int inX, int inY, int inOldBiome, int inNewBiome );
        

        void drawGroundOverlaySprites();
        
        // check which GUI components should be visible
        void checkVisible();

        SceneCell *getCurrentCell();
        SceneCell *getCurrentPersonCell();
        
        // clear everything but biome
        void clearCell( SceneCell *inCell );

        
    };

        


#endif
