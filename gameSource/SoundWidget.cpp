#include "SoundWidget.h"


#include "soundBank.h"


#include "minorGems/util/stringUtils.h"


extern Font *smallFont;



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
          mPrevSubSoundButton( smallFont, -90, 0, "<" ),
          mNextSubSoundButton( smallFont, -46, 0, ">" ),
          mRemoveSubSoundButton( smallFont, -68, 0, "x" ),

          mRecordButton( "recordButton.tga", -85, -30 ),
          mStopButton( "stopButton.tga", -85, -30 ),
          mPlayButton( "playButton.tga", -51, -30 ),
          
          mPlayRandButton( "playButton.tga", -17, 0 ),
          mClearButton( "clearButton.tga", 17, 0 ),
          mCopyButton( "copyButton.tga", 51, 0 ),
          mPasteButton( "pasteButton.tga", 85, 0 ),
          mVolumeSlider( inDisplayFont, -70, -30, 2,
                         124, 20,
                         0, 1.0, "V" ),
          mDefaultVolumeButton( inDisplayFont, 118, -30, "D" ) {

    // put remove under
    addComponent( &mRemoveSubSoundButton );
    addComponent( &mNextSubSoundButton );
    addComponent( &mPrevSubSoundButton );
    
    mRemoveSubSoundButton.setDragOverColor( .80, 0, 0, 1 );
    mRemoveSubSoundButton.setHoverColor( 1.0, 0, 0, 1 );


    nextPrevVisible();
    

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
    
    mNextSubSoundButton.addActionListener( this );
    mPrevSubSoundButton.addActionListener( this );
    mRemoveSubSoundButton.addActionListener( this );


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
    }



void SoundWidget::clearClipboard() {
    unCountLiveUse( sClipboardSoundUsage );
    clearSoundUsage( &sClipboardSoundUsage );
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
    countLiveUse( inUsage );
    
    unCountLiveUse( mSoundUsage );
    clearSoundUsage( &mSoundUsage );


    mCurSoundIndex = 0;
    
    mSoundUsage = copyUsage( inUsage );
    
    
    
        
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

    nextPrevVisible();
    }


        
        
        
void SoundWidget::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mDefaultVolumeButton ) {
        mVolumeSlider.setValue( 0.25 );
        mSoundUsage.volumes[mCurSoundIndex] = 0.25;
        fireActionPerformed( this );
        }
    else if( inTarget == &mVolumeSlider ) {
        mSoundUsage.volumes[mCurSoundIndex] = mVolumeSlider.getValue();
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
        

        SoundUsage oldUsage = copyUsage( mSoundUsage );
        
        int id = stopRecordingSound();
        
        if( id != -1 ) {
            
            if( mCurSoundIndex == mSoundUsage.numSubSounds ) {
                addSound( &mSoundUsage, id, mVolumeSlider.getValue() );
                }
            else {
                mSoundUsage.ids[ mCurSoundIndex ] = id;
                }
            }
        
        countLiveUse( mSoundUsage );

        unCountLiveUse( oldUsage );
        clearSoundUsage( &oldUsage );

        // don't make record visible here
        // wait until next step so that it won't receive this click
        mPlayButton.setVisible( true );
        mPlayRandButton.setVisible( true );
        mCopyButton.setVisible( true );
        mClearButton.setVisible( true );
        
        updatePasteButton();
        nextPrevVisible();
        fireActionPerformed( this );
        }
    else if( inTarget == &mPlayButton ) {
        playSound( mSoundUsage.ids[mCurSoundIndex], mVolumeSlider.getValue() );
        }
    else if( inTarget == &mPlayRandButton ) {
        playSound( mSoundUsage );
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
    else if( inTarget == &mPrevSubSoundButton ) {
        mCurSoundIndex--;
        nextPrevVisible();
        }
    else if( inTarget == &mNextSubSoundButton ) {
        mCurSoundIndex++;
        nextPrevVisible();
        }
    else if( inTarget == &mRemoveSubSoundButton ) {
        
        SoundUsage oldUsage = copyUsage( mSoundUsage );
        
        removeSound( &mSoundUsage, mCurSoundIndex );
        
        if( mCurSoundIndex == oldUsage.numSubSounds - 1 &&
            mCurSoundIndex > 0 ) {
            mCurSoundIndex--;
            }
        
        countLiveUse( mSoundUsage );

        unCountLiveUse( oldUsage );
        clearSoundUsage( &oldUsage );

        nextPrevVisible();
        fireActionPerformed( this );
        }
    
    
    }



void SoundWidget::updatePasteButton() {
    mPasteButton.setVisible( sClipboardSoundUsage.numSubSounds > 0  && 
                             mRecordButton.isVisible() );
    }



void SoundWidget::draw() {    
    markSoundUsageLive( mSoundUsage );
    
    if( ! mRecordButton.isVisible() &&
        ! mStopButton.isVisible() ) {
        // re-enable record button the step after stop is clicked to
        // avoid repeat click
        mRecordButton.setVisible( true );
        updatePasteButton();
        }

    if( mPlayRandButton.isVisible() ) {
        
        if( mPlayRandButton.isMouseOver() ) {
            if( mPlayRandButton.isMouseDragOver() ) {
                setDrawColor( 0.828, 0.647, 0.212, 1 );
                }
            else {
                setDrawColor( 0.886, 0.764, 0.475, 1 );
                }
            }
        else {
            setDrawColor( 1, 1, 1, 1 );            
            }
        
        char *s = autoSprintf( "%d", mSoundUsage.numSubSounds );
        doublePair pos = mPlayRandButton.getPosition();
        pos.y -= 2;
        smallFont->drawString( s, pos, alignCenter );
        delete [] s;
        }
    }



char SoundWidget::isRecording() {
    return ( ! mRecordButton.isVisible() ) && mStopButton.isVisible();
    }



void SoundWidget::nextPrevVisible() {

    if( mSoundUsage.numSubSounds == 0 ) {
        mPrevSubSoundButton.setVisible( false );
        mNextSubSoundButton.setVisible( false );
        mRemoveSubSoundButton.setVisible( false );
        
        mPlayButton.setVisible( false );
        mPlayRandButton.setVisible( false );
        }
    else {
        mPlayRandButton.setVisible( true );
        
        if( mCurSoundIndex < mSoundUsage.numSubSounds ) {
            mRemoveSubSoundButton.setVisible( true );
            mNextSubSoundButton.setVisible( true );
            mPlayButton.setVisible( true );
            }
        else {
            mRemoveSubSoundButton.setVisible( false );
            mNextSubSoundButton.setVisible( false );
            mPlayButton.setVisible( false );    
            }
        
        if( mCurSoundIndex > 0 ) {
            mPrevSubSoundButton.setVisible( true );
            }
        else {
            mPrevSubSoundButton.setVisible( false );
            }
        
        if( mCurSoundIndex < mSoundUsage.numSubSounds - 1 ) {
            // there's a next sound, normal button colors
            mNextSubSoundButton.setDragOverColor( 0.828, 0.647, 0.212, 1 );
            mNextSubSoundButton.setHoverColor( 0.886, 0.764, 0.475, 1 );
            }
        else {
            // green to show that it makes new
            mNextSubSoundButton.setDragOverColor( 0, .80, 0, 1 );
            mNextSubSoundButton.setHoverColor( 0, 1.0, 0, 1 );
            }
        }

    mVolumeSlider.setVisible( mPlayButton.isVisible() );
    mDefaultVolumeButton.setVisible( mPlayButton.isVisible() );

    if( mVolumeSlider.isVisible() ) {
        mVolumeSlider.setValue( mSoundUsage.volumes[ mCurSoundIndex ] );
        }
    }
