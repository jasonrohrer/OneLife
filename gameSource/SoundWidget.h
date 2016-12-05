#ifndef SOUND_WIDGET_INCLUDED
#define SOUND_WIDGET_INCLUDED


#include "SpriteButton.h"
#include "ValueSlider.h"
#include "minorGems/game/Font.h"
#include "minorGems/util/SimpleVector.h"



class SoundWidget : public PageComponent, public ActionListenerList,
                    public ActionListener {
        
    public:
        
        SoundWidget( Font *inDisplayFont, int inX, int inY );
        

        ~SoundWidget();
        

        void setSound( int inSoundID );
        
        int getSound();
        
        double getVolume();
        
        
    protected:
        int mSoundID;
        
        static int sClipboardSound;

        // for propagating clipboard changes
        static SimpleVector<SoundWidget*> sWidgetList;

        SpriteButton mRecordButton;
        SpriteButton mStopButton;
        SpriteButton mPlayButton;
        
        SpriteButton mClearButton;
        
        SpriteButton mCopyButton;
        SpriteButton mPasteButton;
        

        ValueSlider mVolumeSlider;
        
        
        virtual void actionPerformed( GUIComponent *inTarget );


                
        void updatePasteButton();


    };

    


#endif
