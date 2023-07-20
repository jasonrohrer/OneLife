#include "GamePage.h"

#include "TextButton.h"
#include "CheckboxButton.h"
#include "RadioButtonSet.h"
#include "ValueSlider.h"
#include "SoundUsage.h"


#include "minorGems/ui/event/ActionListener.h"




class SettingsPage : public GamePage, public ActionListener {
        
    public:
        
        SettingsPage();
        ~SettingsPage();
        

        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );

        virtual void step();

        virtual void actionPerformed( GUIComponent *inTarget );

        
        virtual void makeActive( char inFresh );
        virtual void makeNotActive();

    protected:
        
        int mOldFullscreenSetting;
        int mOldBorderlessSetting;
        
        SoundUsage mTestSound;

        double mMusicStartTime;


        TextButton mBackButton;
        TextButton mEditAccountButton;
        TextButton mRestartButton;
        TextButton mRedetectButton;

        CheckboxButton mVsyncBox;
        CheckboxButton mFullscreenBox;
        CheckboxButton mBorderlessBox;

        TextField mTargetFrameRateField;


        ValueSlider mMusicLoudnessSlider;
        ValueSlider mSoundEffectsLoudnessSlider;


        CheckboxButton mUseCustomServerBox;
        
        TextField mCustomServerAddressField;
        TextField mCustomServerPortField;

        TextButton mCopyButton;
        TextButton mPasteButton;


        RadioButtonSet *mCursorModeSet;
        
        ValueSlider mCursorScaleSlider;
        

        void checkRestartButtonVisibility();
        
    };
