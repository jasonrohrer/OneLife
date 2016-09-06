#ifndef EDITOR_IMPORT_PAGE_INCLUDED
#define EDITOR_IMPORT_PAGE_INCLUDED




#include "minorGems/ui/event/ActionListener.h"
#include "minorGems/util/SimpleVector.h"

#include "minorGems/game/game.h"


#include "GamePage.h"
#include "TextButton.h"
#include "CheckboxButton.h"
#include "TextField.h"

#include "Picker.h"

#include "ValueSlider.h"


#include "overlayBank.h"

#include "keyLegend.h"



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

        Image *mProcessedShadow;
        SpriteHandle mProcessedShadowSprite;
        
        doublePair mProcessedCenterOffset;

        char mSelectionMultiplicative;


        ValueSlider mShadowSlider;
        CheckboxButton mSolidCheckbox;
        

        TextField mSpriteTagField;
        
        TextButton mSaveSpriteButton;
        TextButton mSaveOverlayButton;

        Picker mSpritePicker;
        Picker mOverlayPicker;
        

        TextButton mObjectEditorButton;

        SpriteHandle mCenterMarkSprite;
        
        char mCenterSet;
        doublePair mCenterPoint;
        

        SimpleVector<doublePair> mOverlayOffset;
        SimpleVector<OverlayRecord *>mCurrentOverlay;
        

        char mMovingSheet;
        doublePair mSheetOffset;
        doublePair mMovingSheetPointerStart;
        

        char mSettingSpriteCenter;
        
        char mMovingOverlay;
        char mScalingOverlay;
        char mRotatingOverlay;
        SimpleVector<double> mOverlayScale;
        SimpleVector<double> mOverlayRotation;
        
        SimpleVector<char> mOverlayFlip;
        

        doublePair mMovingOverlayPointerStart;

        double mMovingOverlayScaleStart;
        double mMovingOverlayRotationStart;

        TextButton mClearRotButton;
        TextButton mClearScaleButton;

        TextButton mFlipOverlayButton;

        TextButton mClearOverlayButton;
        
        KeyLegend mSheetKeyLegend;
        KeyLegend mOverlayKeyLegend;
        
        char mShowTagMessage;
    };



#endif
