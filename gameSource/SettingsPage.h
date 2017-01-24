#include "GamePage.h"

#include "TextButton.h"
#include "CheckboxButton.h"
#include "ValueSlider.h"


#include "minorGems/ui/event/ActionListener.h"




class SettingsPage : public GamePage, public ActionListener {
        
    public:
        
        SettingsPage();
        

        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );

        virtual void actionPerformed( GUIComponent *inTarget );

        
        virtual void makeActive( char inFresh );
        virtual void makeNotActive();

    protected:
        
        int mOldFullscreenSetting;
        

        TextButton mBackButton;
        TextButton mRestartButton;

        CheckboxButton mFullscreenBox;
        

        ValueSlider mMusicLoudnessSlider;
        ValueSlider mSoundEffectsLoudnessSlider;
        
    };
