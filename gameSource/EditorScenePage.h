#ifndef EDITOR_SCENE_PAGE_INCLUDED
#define EDITOR_SCENE_PAGE_INCLUDED


#include "minorGems/ui/event/ActionListener.h"


#include "GamePage.h"

#include "Picker.h"
#include "TextButton.h"



typedef struct SceneCell {
        int biome;
        
        int oID;
        
        SimpleVector<int> contained;
        SimpleVector< SimpleVector<int> > subContained;
        

        char flipH;
        double age;
        
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
        
        SpriteHandle mGroundOverlaySprite[4];

        SceneCell mCells[SCENE_H][SCENE_W];
        
        SceneCell mEmptyCell;
        SceneCell mCopyBuffer;
        

        int mCurX, mCurY;
        
        double mFrameCount;
        
        

        void floodFill( int inX, int inY, int inOldBiome, int inNewBiome );
        

        void drawGroundOverlaySprites();

    };

        


#endif
