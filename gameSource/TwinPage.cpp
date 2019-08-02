#include "TwinPage.h"

#include "message.h"
#include "buttonStyle.h"


#include "minorGems/game/Font.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/crc32.h"

#include "minorGems/game/game.h"


#include <stdio.h>



extern Font *mainFont;
extern char *userEmail;

extern char *userTwinCode;
extern int userTwinCount;



TwinPage::TwinPage()
  : mCodeField( mainFont, 0, 128, 13, false, 
                translate( "twinCode" ),
                NULL,
                NULL ),
    mGenerateButton( mainFont, 0, 49, translate( "generate") ),
    mCopyButton( mainFont, 400, 170, translate( "copy" ) ),
    mPasteButton( mainFont, 400, 86, translate( "paste" ) ),
    mLoginButton( mainFont, 400, -280, translate( "loginButton" ) ),
    mCancelButton( mainFont, -400, -280, 
                   translate( "cancel" ) ) {
    

    setButtonStyle( &mGenerateButton );
    setButtonStyle( &mCopyButton );
    setButtonStyle( &mPasteButton );
    setButtonStyle( &mLoginButton );
    setButtonStyle( &mCancelButton );
    
    addComponent( &mGenerateButton );
    addComponent( &mCopyButton );
    addComponent( &mPasteButton );
    addComponent( &mLoginButton );
    addComponent( &mCancelButton );

    addComponent( &mCodeField );
    
    mGenerateButton.addActionListener( this );
    mCopyButton.addActionListener( this );
    mPasteButton.addActionListener( this );
    mLoginButton.addActionListener( this );
    mCancelButton.addActionListener( this );

    
    mCodeField.addActionListener( this );
    
    mCodeField.setFireOnAnyTextChange( true );
    
    mLoginButton.setVisible( false );
    

    const char *choiceList[3] = { translate( "twins" ),
                                  translate( "triplets" ),
                                  translate( "quadruplets" ) };
    
    mPlayerCountRadioButtonSet = 
        new RadioButtonSet( mainFont, 0, -100,
                            3, choiceList,
                            false, 4 );
    addComponent( mPlayerCountRadioButtonSet );


    if( ! isClipboardSupported() ) {
        mCopyButton.setVisible( false );
        mPasteButton.setVisible( false );
        }


    FILE *f = fopen( "wordList.txt", "r" );
    
    if( f != NULL ) {
    
        int numRead = 1;
        
        char buff[100];
        
        while( numRead == 1 ) {
            numRead = fscanf( f, "%99s", buff );
            
            if( numRead == 1 ) {
                mWordList.push_back( stringDuplicate( buff ) );
                }
            }
        fclose( f );
        }
    
    if( mWordList.size() < 20 ) {
        mGenerateButton.setVisible( false );
        }

    if( userEmail != NULL ) {    
        unsigned int timeSeed = 
            (unsigned int)fmod( game_getCurrentTime(), UINT_MAX );
        unsigned int emailSeed =
            crc32( (unsigned char *)userEmail, strlen( userEmail ) );
        
        mRandSource.reseed( timeSeed + emailSeed );
        }

    char oldSet = false;
    char *oldCode = SettingsManager::getSettingContents( "twinCode", "" );
    
    if( oldCode != NULL ) {
        if( strcmp( oldCode, "" ) != 0 ) {
            mCodeField.setText( oldCode );
            oldSet = true;
            actionPerformed( &mCodeField );
            }
        delete [] oldCode;
        }

    if( !oldSet ) {
        // generate first one automatically
        actionPerformed( &mGenerateButton );
        }
    }


        
TwinPage::~TwinPage() {
    delete mPlayerCountRadioButtonSet;

    mWordList.deallocateStringElements();
    }



void TwinPage::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mCodeField ) {
        char *text = mCodeField.getText();
        
        char *trimText = trimWhitespace( text );

        if( strcmp( trimText, "" ) == 0 ) {
            mLoginButton.setVisible( false );
            }
        else {
            mLoginButton.setVisible( true );
            }
        delete [] text;
        delete [] trimText;
        }
    else if( inTarget == &mCancelButton ) {
        if( userTwinCode != NULL ) {
            delete [] userTwinCode;
            userTwinCode = NULL;
            }
        
        setSignal( "cancel" );
        }
    else if( inTarget == &mGenerateButton ) {
        
        char *pickedWord[3];
        
        for( int i=0; i<3; i++ ) {
            pickedWord[i] = 
                mWordList.getElementDirect( mRandSource.getRandomBoundedInt( 
                                                0, mWordList.size() - 1 ) );
            }
        char *code = autoSprintf( "%s %s %s",
                                  pickedWord[0],
                                  pickedWord[1],
                                  pickedWord[2] );
        
        mCodeField.setText( code );
        actionPerformed( &mCodeField );
        delete [] code;
        }
    else if( inTarget == &mCopyButton ) {
        char *text = mCodeField.getText();
        setClipboardText( text );
        delete [] text;
        }
    else if( inTarget == &mPasteButton ) {
        char *text = getClipboardText();

        mCodeField.setText( text );
        actionPerformed( &mCodeField );

        delete [] text;
        }
    else if( inTarget == &mLoginButton ) {
        if( userTwinCode != NULL ) {
            delete [] userTwinCode;
            userTwinCode = NULL;
            }
        
        char *text = mCodeField.getText();
        
        userTwinCode = trimWhitespace( text );
        delete [] text;

        SettingsManager::setSetting( "twinCode", userTwinCode );

        userTwinCount = mPlayerCountRadioButtonSet->getSelectedItem() + 2;

        setSignal( "done" );
        }
    
    }



void TwinPage::makeActive( char inFresh ) {
    }

        

void TwinPage::draw( doublePair inViewCenter, 
                     double inViewSize ) {
    doublePair pos = { 0, 278 };
    
    drawMessage( translate( "twinTip" ), pos );
    }

