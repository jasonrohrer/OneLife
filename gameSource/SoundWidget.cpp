#include "SoundWidget.h"


#include "soundBank.h"



SoundUsage SoundWidget::sClipboardSoundUsage = { 0, NULL, NULL };


SimpleVector<SoundWidget*> SoundWidget::sWidgetList;


static void styleButton( Button *inButton ) {
    inButton->setPixelSize( 2 );
    //inButton->setSize( inButton->getWidth() / 2, inButton->getHeight() / 2 );
    }



SoundWidget::SoundWidget( Font *inDisplayFont, 
                          int inX, int inY ) 
        : PageComponent( inX, inY ),
          mSoundUsage( blankSoundUsage ),
          mCurSoundIndex( 0 ),
          mRecordButton( "recordButton.tga", -85, -30 ),
          mStopButton( "stopButton.tga", -85, -30 ),
          mPlayButton( "playButton.tga", -51, -30 ),
          
          mPlayRandButton( "playRandButton.tga", -51, 0 ),
          mClearButton( "clearButton.tga", 17, 0 ),
          mCopyButton( "copyButton.tga", 51, 0 ),
          mPasteButton( "pasteButton.tga", 85, 0 ),
          mVolumeSlider( inDisplayFont, -70, -30, 2,
                         124, 20,
                         0, 1.0, "V" ),
          mDefaultVolumeButton( inDisplayFont, 118, -30, "D" ) {

    // stop button receives events before record button
    // this lets them be in same spot (record click already ignored
    // by invisible stop button by time record click causes stop button
    // to become visible)
    addComponent( &mStopButton );
    addComponent( &mRecordButton );
    addComponent( &mPlayButton );

    addComponent( &mPlayRandButton );
    addComponent( &mClearButton );
    addComponent( &mCopyButton );
    addComponent( &mPasteButton );
    addComponent( &mVolumeSlider );
    addComponent( &mDefaultVolumeButton );


    styleButton( &mRecordButton );
    styleButton( &mStopButton );
    styleButton( &mPlayButton );

    styleButton( &mPlayRandButton );
    styleButton( &mClearButton );
    styleButton( &mCopyButton );
    styleButton( &mPasteButton );

    styleButton( &mDefaultVolumeButton );
    
    mVolumeSlider.toggleField( false );

    mVolumeSlider.addActionListener( this );

    mRecordButton.addActionListener( this );
    mStopButton.addActionListener( this );
    mPlayButton.addActionListener( this );

    mPlayRandButton.addActionListener( this );
    mClearButton.addActionListener( this );
    mCopyButton.addActionListener( this );
    mPasteButton.addActionListener( this );

    mDefaultVolumeButton.addActionListener( this );

    mStopButton.setVisible( false );

    setSoundUsage( blankSoundUsage );

    mVolumeSlider.setValue( .25 );

    sWidgetList.push_back( this );

    updatePasteButton();
    }

        

SoundWidget::~SoundWidget() {
    setSoundUsage( blankSoundUsage );
    sWidgetList.deleteElementEqualTo( this );
    
    if( sWidgetList.size() == 0 ) {
        unCountLiveUse( sClipboardSoundUsage );
        clearSoundUsage( &sClipboardSoundUsage );
        }
    }



void SoundWidget::setSoundInternal( SoundUsage inUsage ) {
    setSoundUsage( inUsage );
    fireActionPerformed( this );
    }



SoundUsage SoundWidget::getSoundUsage() {
    if( mStopButton.isVisible() ) {
        return blankSoundUsage;
        }
    return mSoundUsage;
    }



void SoundWidget::setSoundUsage( SoundUsage inUsage ) {
    unCountLiveUse( mSoundUsage );
    clearSoundUsage( &mSoundUsage );
    
    mSoundUsage = copyUsage( inUsage );
    countLiveUse( mSoundUsage );
    
    
        
    char show = ( mSoundUsage.numSubSounds > 0 );
    
    mPlayButton.setVisible( show );
    mPlayRandButton.setVisible( show );
    mVolumeSlider.setVisible( show );
    mCopyButton.setVisible( show );
    mClearButton.setVisible( show );
    mDefaultVolumeButton.setVisible( show );
    

    if( mSoundUsage.numSubSounds > mCurSoundIndex ) {
        mVolumeSlider.setValue( mSoundUsage.volumes[ mCurSoundIndex ] );
        }
    }


        
        
        
void SoundWidget::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mDefaultVolumeButton ) {
        mVolumeSlider.setValue( 0.25 );
        fireActionPerformed( this );
        }
    else if( inTarget == &mVolumeSlider ) {
        fireActionPerformed( this );
        }
    else if( inTarget == &mRecordButton ) {
        char started = startRecordingSound();
        
        mStopButton.setVisible( started );
        mRecordButton.setVisible( !started );
        
        if( started ) {
            mPlayButton.setVisible( false );
            mPlayRandButton.setVisible( false );
            mCopyButton.setVisible( false );
            mClearButton.setVisible( false );
            }
        updatePasteButton();
        fireActionPerformed( this );
        }
    else if( inTarget == &mStopButton ) {
        mStopButton.setVisible( false );
        
        int id = stopRecordingSound();
        
        if( id != -1 ) {
            
            if( mCurSoundIndex == mSoundUsage.numSubSounds ) {
                addSound( &mSoundUsage, id, mVolumeSlider.getValue() );
                }
            else {
                mSoundUsage.ids[ mCurSoundIndex ] = id;
                }
            }
        
        // don't make record visible here
        // wait until next step so that it won't receive this click
        mPlayButton.setVisible( true );
        mPlayRandButton.setVisible( true );
        mCopyButton.setVisible( true );
        mClearButton.setVisible( true );
        
        updatePasteButton();
        fireActionPerformed( this );
        }
    else if( inTarget == &mPlayButton ) {
        playSound( mSoundUsage.ids[mCurSoundIndex], 
                   mVolumeSlider.getValue(), 0.5 );
        }
    else if( inTarget == &mClearButton ) {
        setSoundInternal( blankSoundUsage );
        }
    else if( inTarget == &mCopyButton ) {
        if( mSoundUsage.numSubSounds > 0 && 
            ! equal( mSoundUsage, sClipboardSoundUsage ) ) {

            unCountLiveUse( sClipboardSoundUsage );
            clearSoundUsage( &sClipboardSoundUsage );

            sClipboardSoundUsage = copyUsage( mSoundUsage );
            
            countLiveUse( sClipboardSoundUsage );
            
            for( int i=0; i<sWidgetList.size(); i++ ) {
                sWidgetList.getElementDirect(i)->updatePasteButton();
                }
            }
        }
    else if( inTarget == &mPasteButton ) {
        if( sClipboardSoundUsage.numSubSounds > 0 ) {
            mCurSoundIndex = 0;
            setSoundInternal( sClipboardSoundUsage );
            }
        }
    }



void SoundWidget::updatePasteButton() {
    mPasteButton.setVisible( sClipboardSoundUsage.numSubSounds > 0  && 
                             mRecordButton.isVisible() );
    }



void SoundWidget::draw() {    
    if( mSoundID != -1 ) {
        markSoundUsageLive( mSoundUsage );
        }

    if( ! mRecordButton.isVisible() &&
        ! mStopButton.isVisible() ) {
        // re-enable record button the step after stop is clicked to
        // avoid repeat click
        mRecordButton.setVisible( true );
        }
    }



char SoundWidget::isRecording() {
    return ( ! mRecordButton.isVisible() ) && mStopButton.isVisible();
    }
