#include "GamePage.h"

#include "TextButton.h"
#include "CheckboxButton.h"
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
        
        SoundUsage mTestSound;

        double mMusicStartTime;


        TextButton mBackButton;
        TextButton mRestartButton;
        TextButton mRedetectButton;

        CheckboxButton mFullscreenBox;
        

        ValueSlider mMusicLoudnessSlider;
        ValueSlider mSoundEffectsLoudnessSlider;
        
    };
