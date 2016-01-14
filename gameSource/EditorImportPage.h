#ifndef EDITOR_IMPORT_PAGE_INCLUDED
#define EDITOR_IMPORT_PAGE_INCLUDED




#include "minorGems/ui/event/ActionListener.h"
#include "minorGems/util/SimpleVector.h"

#include "minorGems/game/game.h"


#include "GamePage.h"
#include "TextButton.h"
#include "TextField.h"

#include "Picker.h"


#include "overlayBank.h"



class EditorImportPage : public GamePage, public ActionListener {
        
    public:
        EditorImportPage();
        ~EditorImportPage();

        void clearUseOfOverlay( int inOverlayID );

        
        virtual void actionPerformed( GUIComponent *inTarget );
        
        virtual void drawUnderComponents( doublePair inViewCenter, 
                                          double inViewSize );
        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );
        
        virtual void step();
  
        virtual void makeActive( char inFresh );
        

        virtual void pointerMove( float inX, float inY );
        virtual void pointerDown( float inX, float inY );
        virtual void pointerDrag( float inX, float inY );
        virtual void pointerUp( float inX, float inY );

        virtual void keyDown( unsigned char inASCII );
        virtual void keyUp( unsigned char inASCII );


    protected:
        

        void processSelection();
        

        TextButton mImportButton;
        TextButton mImportOverlayButton;

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
        TextButton mSaveOverlayButton;

        Picker mSpritePicker;
        Picker mOverlayPicker;
        

        TextButton mObjectEditorButton;

        SpriteHandle mCenterMarkSprite;
        
        char mCenterSet;
        doublePair mCenterPoint;
        

        doublePair mOverlayOffset;
        OverlayRecord *mCurrentOverlay;
        

        char mMovingOverlay;
        char mScalingOverlay;
        double mOverlayScale;
        
        doublePair mMovingOverlayPointerStart;

        double mMovingOverlayScaleStart;
    };



#endif
