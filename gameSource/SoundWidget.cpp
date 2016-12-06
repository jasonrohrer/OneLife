#include "SoundWidget.h"


#include "soundBank.h"



int SoundWidget::sClipboardSound = -1;


SimpleVector<SoundWidget*> SoundWidget::sWidgetList;


static void styleButton( Button *inButton ) {
    inButton->setPixelSize( 2 );
    //inButton->setSize( inButton->getWidth() / 2, inButton->getHeight() / 2 );
    }



SoundWidget::SoundWidget( Font *inDisplayFont, 
                          int inX, int inY ) 
        : PageComponent( inX, inY ),
          mSoundID( -1 ),
          mRecordButton( "recordButton.tga", -85, 0 ),
          mStopButton( "stopButton.tga", -51, 0 ),
          mPlayButton( "playButton.tga", -17, 0 ),
          mClearButton( "clearButton.tga", 17, 0 ),
          mCopyButton( "copyButton.tga", 51, 0 ),
          mPasteButton( "pasteButton.tga", 85, 0 ),
          mVolumeSlider( inDisplayFont, -148, -30, 2,
                         202, 20,
                         0, 1.0, "V" ) {

    addComponent( &mRecordButton );
    addComponent( &mStopButton );
    addComponent( &mPlayButton );
    addComponent( &mClearButton );
    addComponent( &mCopyButton );
    addComponent( &mPasteButton );
    addComponent( &mVolumeSlider );


    styleButton( &mRecordButton );
    styleButton( &mStopButton );
    styleButton( &mPlayButton );
    styleButton( &mClearButton );
    styleButton( &mCopyButton );
    styleButton( &mPasteButton );
    
    mVolumeSlider.toggleField( false );

    mVolumeSlider.addActionListener( this );

    mRecordButton.addActionListener( this );
    mStopButton.addActionListener( this );
    mPlayButton.addActionListener( this );
    mClearButton.addActionListener( this );
    mCopyButton.addActionListener( this );
    mPasteButton.addActionListener( this );

    mStopButton.setVisible( false );

    setSound( -1 );

    mVolumeSlider.setValue( 1 );

    sWidgetList.push_back( this );

    updatePasteButton();
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
    if( inSoundID != -1 ) {
        countLiveUse( inSoundID );
        }

    if( mSoundID != -1 ) {
        unCountLiveUse( mSoundID );
        }

    mSoundID = inSoundID;
    
    
    mPlayButton.setVisible( mSoundID != -1 );
    mVolumeSlider.setVisible( mSoundID != -1 );
    mCopyButton.setVisible( mSoundID != -1 );
    mClearButton.setVisible( mSoundID != -1 );
    }



void SoundWidget::setSoundInternal( int inSoundID ) {
    setSound( inSoundID );
    fireActionPerformed( this );
    }



        
int SoundWidget::getSound() {
    return mSoundID;
    }



double SoundWidget::getVolume() {
    return mVolumeSlider.getValue();
    }



SoundUsage SoundWidget::getSoundUsage() {
    SoundUsage s = { mSoundID, mVolumeSlider.getValue() };
    return s;
    }



void SoundWidget::setSoundUsage( SoundUsage inUsage ) {
    setSound( inUsage.id );
    mVolumeSlider.setValue( inUsage.volume );
    }


        
        
        
void SoundWidget::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mVolumeSlider ) {
        fireActionPerformed( this );
        }
    else if( inTarget == &mRecordButton ) {
        setSoundInternal( -1 );

        char started = startRecordingSound();
        
        mStopButton.setVisible( started );
        mRecordButton.setVisible( !started );
        updatePasteButton();
        }
    else if( inTarget == &mStopButton ) {
        mStopButton.setVisible( false );
        
        int id = stopRecordingSound();
        
        setSoundInternal( id );
        
        mRecordButton.setVisible( true );
        updatePasteButton();
        }
    else if( inTarget == &mPlayButton ) {
        playSound( mSoundID, mVolumeSlider.getValue(), 0.5 );
        }
    else if( inTarget == &mClearButton ) {
        setSoundInternal( -1 );
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
            setSoundInternal( sClipboardSound );
            }
        }
    }



void SoundWidget::updatePasteButton() {
    mPasteButton.setVisible( sClipboardSound != -1  && 
                             mRecordButton.isVisible() );
    }



void SoundWidget::draw() {    
    if( mSoundID != -1 ) {
        markSoundLive( mSoundID );
        }
    }

