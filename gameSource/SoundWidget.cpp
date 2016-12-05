#include "SoundWidget.h"


#include "soundBank.h"



int SoundWidget::sClipboardSound = -1;


SimpleVector<SoundWidget*> SoundWidget::sWidgetList;



SoundWidget::SoundWidget( Font *inDisplayFont, 
                          int inX, int inY ) 
        : PageComponent( inX, inY ),
          mSoundID( -1 ),
          mRecordButton( "recordButton.tga", -64, 0 ),
          mStopButton( "stopButton.tga", -32, 0 ),
          mPlayButton( "playButton.tga", 0, 0 ),
          mClearButton( "clearButton.tga", 32, 0 ),
          mCopyButton( "copyButton.tga", 64, 0 ),
          mPasteButton( "pasteButton.tga", 96, 0 ),
          mVolumeSlider( inDisplayFont, 0, -64, 2,
                         50, 20,
                         0, 1.0, "V" ) {

    addComponent( &mRecordButton );
    addComponent( &mStopButton );
    addComponent( &mPlayButton );
    addComponent( &mClearButton );
    addComponent( &mCopyButton );
    addComponent( &mPasteButton );
    addComponent( &mVolumeSlider );
    
    mRecordButton.addActionListener( this );
    mStopButton.addActionListener( this );
    mPlayButton.addActionListener( this );
    mClearButton.addActionListener( this );
    mCopyButton.addActionListener( this );
    mPasteButton.addActionListener( this );

    mStopButton.setVisible( false );
    mPlayButton.setVisible( false );
    mClearButton.setVisible( false );
    mCopyButton.setVisible( false );

    mVolumeSlider.setValue( 1 );

    sWidgetList.push_back( this );
    }

        

SoundWidget::~SoundWidget() {
    setSound( -1 );
    sWidgetList.deleteElementEqualTo( this );
    
    if( sWidgetList.size() == 0 ) {
        if( sClipboardSound != -1 ) {
            unCountLiveUse( sClipboardSound );
            sClipboardSound = -1;
            }
        }
    }



void SoundWidget::setSound( int inSoundID ) {
    if( mSoundID != -1 ) {
        unCountLiveUse( mSoundID );
        }

    mSoundID = inSoundID;
    
    if( mSoundID != -1 ) {
        countLiveUse( mSoundID );
        }
    
    mPlayButton.setVisible( mSoundID != -1 );
    mCopyButton.setVisible( mSoundID != -1 );
    mClearButton.setVisible( mSoundID != -1 );
    }


        
int SoundWidget::getSound() {
    return mSoundID;
    }


double SoundWidget::getVolume() {
    return mVolumeSlider.getValue();
    }

        
        
        
void SoundWidget::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mRecordButton ) {
        setSound( -1 );

        char started = startRecordingSound();
        
        mStopButton.setVisible( started );
        mPlayButton.setVisible( false );
        mRecordButton.setVisible( !started );
        mCopyButton.setVisible( false );
        mPlayButton.setVisible( !started );
        mClearButton.setVisible( false );
        mPasteButton.setVisible( false );
        }
    else if( inTarget == &mStopButton ) {
        mStopButton.setVisible( false );
        
        int id = stopRecordingSound();
        
        setSound( id );
        
        mRecordButton.setVisible( true );
        updatePasteButton();
        }
    else if( inTarget == &mPlayButton ) {
        playSound( mSoundID, mVolumeSlider.getValue(), 0.5 );
        }
    else if( inTarget == &mClearButton ) {
        setSound( -1 );
        }
    else if( inTarget == &mCopyButton ) {
        if( mSoundID != -1 && mSoundID != sClipboardSound ) {
            if( sClipboardSound != -1 ) {
                unCountLiveUse( sClipboardSound );
                }
            sClipboardSound = mSoundID;
            countLiveUse( sClipboardSound );
            
            for( int i=0; i<sWidgetList.size(); i++ ) {
                sWidgetList.getElementDirect(i)->updatePasteButton();
                }
            }
        }
    else if( inTarget == &mPasteButton ) {
        if( sClipboardSound != -1 ) {
            setSound( sClipboardSound );
            }
        }
    }



void SoundWidget::updatePasteButton() {
    mPasteButton.setVisible( sClipboardSound != -1  && 
                             mRecordButton.isVisible() );
    }
