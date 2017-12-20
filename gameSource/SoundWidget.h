#ifndef SOUND_WIDGET_INCLUDED
#define SOUND_WIDGET_INCLUDED


#include "SpriteButton.h"
#include "TextButton.h"
#include "ValueSlider.h"
#include "minorGems/game/Font.h"
#include "minorGems/util/SimpleVector.h"

#include "SoundUsage.h"


// action fired when sound or volume changes internally
class SoundWidget : public PageComponent, public ActionListenerList,
                    public ActionListener {
        
    public:
        
        SoundWidget( Font *inDisplayFont, int inX, int inY );
        

        ~SoundWidget();
        
        static void clearClipboard();
        
        
        // returns internal copy
        // not deallocated by caller
        // returns blankSoundUsage during recording
        SoundUsage getSoundUsage();

        // copies internally
        void setSoundUsage( SoundUsage inUsage );

        char isRecording();
        
    protected:

        SoundUsage mSoundUsage;
        
        // subsound index in Usage
        // if == numSubSounds, then we are recording a new subsount
        int mCurSoundIndex;
        

        void setSoundInternal( SoundUsage inUsage );

        
        static SoundUsage sClipboardSoundUsage;
        
        // for propagating clipboard changes
        static SimpleVector<SoundWidget*> sWidgetList;

        TextButton mPrevSubSoundButton;
        TextButton mNextSubSoundButton;
        TextButton mRemoveSubSoundButton;
        
        
        SpriteButton mRecordButton;
        SpriteButton mStopButton;
        SpriteButton mPlayButton;
        

        SpriteButton mPlayRandButton;
        SpriteButton mClearButton;
        
        SpriteButton mCopyButton;
        SpriteButton mPasteButton;
        

        ValueSlider mVolumeSlider;
        
        TextButton mDefaultVolumeButton;

        
        virtual void actionPerformed( GUIComponent *inTarget );


        virtual void draw();
        
                
        void updatePasteButton();


        void nextPrevVisible();
        

    };

    


#endif
