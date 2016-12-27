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
        

        void setSound( int inSoundID );
        
        int getSound();
        
        double getVolume();
        
        SoundUsage getSoundUsage();

        void setSoundUsage( SoundUsage inUsage );

        char isRecording();
        
    protected:

        void setSoundInternal( int inSoundID );


        int mSoundID;
        
        static int sClipboardSound;
        static double sClipboardVolume;
        
        // for propagating clipboard changes
        static SimpleVector<SoundWidget*> sWidgetList;
        
        SpriteButton mRecordButton;
        SpriteButton mStopButton;
        SpriteButton mPlayButton;
        
        SpriteButton mClearButton;
        
        SpriteButton mCopyButton;
        SpriteButton mPasteButton;
        

        ValueSlider mVolumeSlider;
        
        TextButton mDefaultVolumeButton;

        
        virtual void actionPerformed( GUIComponent *inTarget );


        virtual void draw();
        
                
        void updatePasteButton();


    };

    


#endif
