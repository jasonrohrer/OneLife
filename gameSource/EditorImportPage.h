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
        virtual void specialKeyDown( int inKeyCode );

    protected:
        

        void processSelection();
        

        void clearLines();
        

        TextButton mImportButton;
        TextButton mImportLinesButton;
        
        TextButton mNextSpriteImportButton;
        TextButton mPrevSpriteImportButton;

        TextButton mNextLinesImportButton;
        TextButton mPrevLinesImportButton;
        
        int mCurrentSpriteImportCacheIndex;
        int mCurrentLinesImportCacheIndex;
        
        char *mImportPathOverride;

        
        TextButton mXTopLinesButton;
        TextButton mImportOverlayButton;




        int mSheetW, mSheetH;

        char mSelect;
        doublePair mSelectStart;
        doublePair mSelectEnd;
        
        Image *mImportedSheet;
        SpriteHandle mImportedSheetSprite;
        
        // starts transparent
        // we add white to it with mouse
        Image *mWhiteOutSheet;
        SpriteHandle mWhiteOutSheetSprite;


        Image *mProcessedSelection;
        SpriteHandle mProcessedSelectionSprite;

        Image *mProcessedShadow;
        SpriteHandle mProcessedShadowSprite;
        
        doublePair mProcessedCenterOffset;

        char mSelectionMultiplicative;


        ValueSlider mShadowSlider;
        CheckboxButton mSolidCheckbox;
        
        ValueSlider mBlackLineThresholdSlider;
        TextButton mBlackLineThresholdDefaultButton;

        ValueSlider mSaturationSlider;
        TextButton mSaturationDefaultButton;


        TextField mSpriteTagField;
        
        TextButton mSaveSpriteButton;
        TextButton mSaveOverlayButton;

        Picker mSpritePicker;
        Picker mOverlayPicker;
        

        TextButton mSpriteTrimEditorButton;
        TextButton mObjectEditorButton;

        SpriteHandle mCenterMarkSprite;
        SpriteHandle mInternalPaperMarkSprite;
        
        char mCenterSet;
        doublePair mCenterPoint;
        
        SimpleVector<doublePair> mInternalPaperPoints;
        

        SimpleVector<doublePair> mOverlayOffset;
        SimpleVector<OverlayRecord *>mCurrentOverlay;
        

        SimpleVector<doublePair> mLinesOffset;
        SimpleVector<Image*> mLinesImages;
        SimpleVector<SpriteHandle> mLinesSprites;
        

        char mMovingSheet;
        doublePair mSheetOffset;
        doublePair mMovingSheetPointerStart;
        

        char mSettingSpriteCenter;
        
        char mWhitingOut;
        
        char mAnyWhiteOutSet;
        
        char mPlacingInternalPaper;
        

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
        KeyLegend mSheetKeyLegendB;
        KeyLegend mLinesKeyLegend;
        KeyLegend mOverlayKeyLegend;
        
        char mShowTagMessage;
    };



#endif
