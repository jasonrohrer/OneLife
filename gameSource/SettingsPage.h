#include "GamePage.h"

#include "TextButton.h"
#include "CheckboxButton.h"
#include "RadioButtonSet.h"
#include "ValueSlider.h"
#include "SoundUsage.h"
#include "DropdownList.h"


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
        int mEnableNudeSetting;
        int mEnableFOVSetting;
        int mEnableKActionsSetting;
        int mEnableCenterCameraSetting;
        
        SoundUsage mTestSound;

        double mMusicStartTime;


        TextButton mInfoSeeds;
        TextButton mBackButton;
        TextButton mEditAccountButton;
        TextButton mRestartButton;
        TextButton mRedetectButton;

        CheckboxButton mFullscreenBox;
        CheckboxButton mBorderlessBox;
        
		CheckboxButton mEnableNudeBox;
		CheckboxButton mEnableFOVBox;
		CheckboxButton mEnableKActionsBox;
		CheckboxButton mEnableCenterCameraBox;

        ValueSlider mMusicLoudnessSlider;
        ValueSlider mSoundEffectsLoudnessSlider;


        DropdownList mSpawnSeed;

        RadioButtonSet *mCursorModeSet;
        
        ValueSlider mCursorScaleSlider;

    };
