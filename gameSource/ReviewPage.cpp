#include "ReviewPage.h"


#include "minorGems/game/Font.h"

#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/game/game.h"

#include "minorGems/network/web/URLUtils.h"
#include "minorGems/crypto/hashes/sha1.h"


#include "buttonStyle.h"
#include "spellCheck.h"
#include "accountHmac.h"


extern Font *mainFont;
extern Font *mainFontReview;

extern char *userEmail;



ReviewPage::ReviewPage( const char *inReviewServerURL )
        : ServerActionPage( inReviewServerURL, "submit_review", false ),
          mReviewNameField( mainFont, -242, 250, 10, false,
                            translate( "reviewName"), 
                            "abcdefghijklmnopqrstuvwxyz"
                            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                            "1234567890"
                            " ._-" ),
          mReviewTextArea( 
              mainFont, mainFontReview, 0, 0, 800, 320, false, 
              translate( "reviewText" ), 
              "abcdefghijklmnopqrstuvwxyz"
              "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
              "1234567890"
              " !?$%*&()+-='\":;,.\r", NULL ),
          mSpellcheckButton( 396, -200, 4 ),
          mBackButton( mainFont, -526, -140, translate( "backButton" ) ),
          mPostButton( mainFont, 526, -140, translate( "postButton" ) ),
          mRemoveButton( mainFont, 526, -40, translate( "removeButton" ) ),
          mCopyButton( mainFont, -526, 140, translate( "copy" ) ),
          mPasteButton( mainFont, -526, 40, translate( "paste" ) ),
          mClearButton( mainFont, 526, 140, translate( "clear" ) ),
          mGettingSequenceNumber( false ),
          mRemoving( false ) {

    const char *choiceList[2] = { translate( "recommendYes" ),
                                  translate( "recommendNo" ) };
    
    mRecommendChoice = 
        new RadioButtonSet( mainFont, 396, 258,
                            2, choiceList,
                            false, 4 ),
    
    
    setButtonStyle( &mBackButton );
    setButtonStyle( &mPostButton );
    setButtonStyle( &mRemoveButton );
    setButtonStyle( &mCopyButton );
    setButtonStyle( &mPasteButton );
    setButtonStyle( &mClearButton );
    

    addComponent( &mBackButton );
    addComponent( &mPostButton );
    addComponent( &mRemoveButton );

    addComponent( &mCopyButton );
    addComponent( &mPasteButton );
    addComponent( &mClearButton );

        

    mBackButton.addActionListener( this );
    mPostButton.addActionListener( this );
    mRemoveButton.addActionListener( this );
    mCopyButton.addActionListener( this );
    mPasteButton.addActionListener( this );
    mClearButton.addActionListener( this );
    
    // add name field after so we can hit return in name field
    // and advance to text area without sending a return key to the text area
    addComponent( &mReviewTextArea );    
    addComponent( &mSpellcheckButton );
    
    addComponent( &mReviewNameField );

    addComponent( mRecommendChoice );


    mSpellcheckButton.addActionListener( this );

    mReviewNameField.setMaxLength( 20 );
    mReviewTextArea.setMaxLength( 5000 );

    mReviewNameField.addActionListener( this );

    mReviewNameField.setLabelTop( true );


    mPostButton.setMouseOverTip( translate( "postReviewTip" ) );
    mRemoveButton.setMouseOverTip( translate( "removeReviewTip" ) );


    mCopyButton.setMouseOverTip( translate( "copyReviewTip" ) );


    if( ! isClipboardSupported() ) {
        mCopyButton.setVisible( false );
        mPasteButton.setVisible( false );
        }
    
    }



ReviewPage::~ReviewPage() {
    delete mRecommendChoice;
    
    if( isSpellCheckReady() ) {
        freeSpellCheck();
        }
    }



void ReviewPage::saveReview() {
    char *reviewName = mReviewNameField.getText();
    char *reviewText = mReviewTextArea.getText();
    
    int reviewRecommend = 0;
    if( mRecommendChoice->getSelectedItem() == 0 ) {
        reviewRecommend = 1;
        }

    SettingsManager::setSetting( "reviewName", reviewName );
    SettingsManager::setSetting( "reviewText", reviewText );
    SettingsManager::setSetting( "reviewRecommend", reviewRecommend );
    
    delete [] reviewName;
    delete [] reviewText;
    }



void ReviewPage::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mBackButton ) {        
        saveReview();
        
        if( mWebRequest != -1 ) {
            clearWebRequest( mWebRequest );
            mWebRequest = -1;
            }
        
        setSignal( "back" );
        }
    else if( inTarget == &mReviewNameField ) {
        switchFields();
        }
    else if( inTarget == &mCopyButton ) {
        mReviewTextArea.focus();
        
        char *text;
        if( mReviewTextArea.isAnythingSelected() ) {
            text = mReviewTextArea.getSelectedText();
            }
        else {
            text = mReviewTextArea.getText();
            }
        
        setClipboardText( text );
        delete [] text;
        }
    else if( inTarget == &mPasteButton ) {
        mReviewTextArea.focus();

        char *text = getClipboardText();

        int len = strlen( text );
        
        for( int i=0; i<len; i++ ) {
            if( text[i] == '\n' ) {
                text[i] = '\r';
                }
            }
        
        mReviewTextArea.insertString( text );
        delete [] text;
        }
    else if( inTarget == &mClearButton ) {
        mReviewTextArea.focus();

        char *text = stringDuplicate( "" );

        mReviewTextArea.setText( text );
        delete [] text;
        }
    else if( inTarget == &mSpellcheckButton ) {
        char spellCheck = mSpellcheckButton.getToggled();
        
        mReviewTextArea.enableSpellCheck( spellCheck );
        SettingsManager::setSetting( "spellCheckOn", (int)spellCheck );
        }
    else if( inTarget == &mPostButton ) {
        saveReview();
        mCopyButton.setActive( false );
        mPasteButton.setActive( false );
        mClearButton.setActive( false );
        mReviewNameField.setActive( false );
        mReviewTextArea.setActive( false );
        mRecommendChoice->setActive( false );
        mSpellcheckButton.setActive( false );
        
        mRemoveButton.setVisible( false );
        mPostButton.setVisible( false );

        setActionName( "submit_review" );
        
        clearActionParameters();
        
        
        char *encodedEmail = URLUtils::urlEncode( userEmail );
        setActionParameter( "email", encodedEmail );
        delete [] encodedEmail;
        
        int reviewScore = 0;
        if( mRecommendChoice->getSelectedItem() == 0 ) {
            reviewScore = 1;
            }
        
        setActionParameter( "review_score", reviewScore );
        
        char *name = mReviewNameField.getText();
        
        char *encodedName = URLUtils::urlEncode( name );
        

        setActionParameter( "review_name", encodedName );
        delete [] encodedName;
        

        char *text = mReviewTextArea.getText();
        
        int textLen = strlen( text );
        for( int i=0; i<textLen; i++ ) {
            if( text[i] == '\r' ) {
                text[i] = '\n';
                }
            }
        

        char *encodedText = URLUtils::urlEncode( text );
        
        setActionParameter( "review_text", encodedText );
        delete [] encodedText;
        
        char *textSHA1 = computeSHA1Digest( text );

        

        char *nameSHA1 = computeSHA1Digest( name );

        
        
        char *stringToHash = autoSprintf( "%d%s%s",
                                          reviewScore, nameSHA1, textSHA1 );

        delete [] name;
        delete [] text;
        delete [] textSHA1;
        delete [] nameSHA1;
        

        char *pureKey = getPureAccountKey();
        
        char *hash = hmac_sha1( pureKey, stringToHash );
        
        delete [] pureKey;
        delete [] stringToHash;
        

        setActionParameter( "hash_value", hash );
        
        delete [] hash;
        


        mGettingSequenceNumber = false;
        mRemoving = false;
        startRequest();
        }
    else if( inTarget == &mRemoveButton ) {
        saveReview();

        mCopyButton.setActive( false );
        mPasteButton.setActive( false );
        mClearButton.setActive( false );
        mReviewNameField.setActive( false );
        mReviewTextArea.setActive( false );
        mRecommendChoice->setActive( false );
        mSpellcheckButton.setActive( false );

        mRemoveButton.setVisible( false );
        mPostButton.setVisible( false );

        setActionName( "get_sequence_number" );
        
        clearActionParameters();

        char *encodedEmail = URLUtils::urlEncode( userEmail );
        setActionParameter( "email", encodedEmail );
        delete [] encodedEmail;
        
        mGettingSequenceNumber = true;
        mRemoving = false;
        startRequest();
        }
    }



void ReviewPage::draw( doublePair inViewCenter, 
                         double inViewSize ) {

    doublePair pos = mRecommendChoice->getPosition();
    
    pos.y += 32;
    pos.x += 12;
    
    setDrawColor( 1, 1, 1, 1 );
    mainFont->drawString( translate( "recommend" ), pos, alignRight );


    if( mReviewTextArea.isAtLimit() ) {
        
        pos.x = 0;
        pos.y = -193;
        mainFont->drawString( translate( "charLimit" ), pos, alignCenter );
        }
    
    pos = mSpellcheckButton.getPosition();
    
    pos.x -= 24;
    pos.y -= 2;
    
    mainFont->drawString( translate( "spellCheck" ), pos, alignRight );
    }




void ReviewPage::checkCanPost() {
    if( mReviewNameField.isActive() ) {
        // make post button inactive until they at least type a name

        char *text = mReviewNameField.getText();
        
        int len = strlen( text );

        char hide = false;
        
        if( len == 0 ) {
            hide = true;
            }
        else {
            char nonSpace = false;
            for( int i=0; i<len; i++ ) {
                if( text[i] != ' ' ) {
                    nonSpace = true;
                    break;
                    }
                }

            if( !nonSpace ) {
                hide = true;
                }
            }
        
        mPostButton.setVisible( ! hide );

        delete [] text;
        }
    }



void ReviewPage::checkCanPaste() {
    if( mReviewTextArea.isActive() ) {
        
        char foc = mReviewTextArea.isFocused();

        char clipSupport = isClipboardSupported();

        mCopyButton.setVisible( foc && clipSupport );
        mPasteButton.setVisible( foc && clipSupport );
        mClearButton.setVisible( foc );

        if( mReviewTextArea.isAnythingSelected() ) {
            mCopyButton.setMouseOverTip( translate( "copySelectionTip" ) );
            }
        else {
            mCopyButton.setMouseOverTip( translate( "copyReviewTip" ) );
            }
        }
    }




void ReviewPage::step() {
    checkCanPost();
    checkCanPaste();

    ServerActionPage::step();

    if( isResponseReady() ) {

        if( ! mGettingSequenceNumber && ! mRemoving ) {
            
            int reviewPosted = SettingsManager::getIntSetting( 
                "reviewPosted", 0 );
            
            if( reviewPosted ) {
                setStatus( "reviewUpdated", false );
                }
            else {
                setStatus( "reviewPosted", false );
                }
        
            SettingsManager::setSetting( "reviewPosted", 1 );
            }
        else if( ! mRemoving ) {
            
            char *seqString = getResponse( 0 );
            
            if( seqString != NULL ) {
                int seq = 0;
                sscanf( seqString, "%d", &seq );
                
                delete [] seqString;
                

                mResponseReady = false;
                
                setActionName( "remove_review" );
        
                clearActionParameters();
        
        
                char *encodedEmail = URLUtils::urlEncode( userEmail );
                setActionParameter( "email", encodedEmail );
                delete [] encodedEmail;
        
                setActionParameter( "sequence_number", seq );
                
        
        
                char *stringToHash = autoSprintf( "%d", seq );

                
                char *pureKey = getPureAccountKey();
                
                char *hash = hmac_sha1( pureKey, stringToHash );
                
                delete [] pureKey;
                delete [] stringToHash;
                
                
                setActionParameter( "hash_value", hash );
        
                delete [] hash;
        


                mGettingSequenceNumber = false;
                mRemoving = true;
                startRequest();
                }
            }
        else if( mRemoving ) {
            setStatus( "reviewRemoved", false );
            
            SettingsManager::setSetting( "reviewPosted", 0 );
            }
        }
    }





void ReviewPage::makeActive( char inFresh ) {
    if( inFresh ) {        
        }

    if( ! isSpellCheckReady() ) {
        initSpellCheck();
        }
    
    setStatus( NULL, false );
    mResponseReady = false;
    

    mReviewNameField.setActive( true );
    mReviewTextArea.setActive( true );
    mRecommendChoice->setActive( true );
    mSpellcheckButton.setActive( true );
    

    mCopyButton.setActive( true );
    mPasteButton.setActive( true );
    mClearButton.setActive( true );

    
    char spellCheck = SettingsManager::getIntSetting( "spellCheckOn", 1 );
    
    mReviewTextArea.enableSpellCheck( spellCheck );

    mSpellcheckButton.setToggled( spellCheck );
    

    mReviewNameField.focus();


    char *reviewName = SettingsManager::getSettingContents( "reviewName", "" );
    char *reviewText = SettingsManager::getSettingContents( "reviewText", "" );
    
    int reviewRecommend = 
        SettingsManager::getIntSetting( "reviewRecommend", 1 );
    
    if( reviewRecommend == 0 ) {    
        mRecommendChoice->setSelectedItem( 1 );
        }
    else {
        mRecommendChoice->setSelectedItem( 0 );
        }
    
    mReviewNameField.setText( reviewName );

    char *oldText = mReviewTextArea.getText();
    
    if( strcmp( oldText, reviewText ) != 0 ) {    
        // keep cursor pos if text hasn't changed
        mReviewTextArea.setText( reviewText );
        }
    
    delete [] oldText;
    

    delete [] reviewName;
    delete [] reviewText;


        
    int reviewPosted = SettingsManager::getIntSetting( "reviewPosted", 0 );
    
    if( reviewPosted ) {
        mPostButton.setLabelText( translate( "updateButton" ) );
        mPostButton.setMouseOverTip( translate( "updateReviewTip" ) );
        mRemoveButton.setVisible( true );
        }
    else {
        mPostButton.setLabelText( translate( "postButton" ) );
        mPostButton.setMouseOverTip( translate( "postReviewTip" ) );
        mRemoveButton.setVisible( false );
        }

    

    checkCanPost();

    
    if( mPostButton.isVisible() ) {
        // name field has something already
        // put focus on review text
        mReviewTextArea.focus();
        }
    

    checkCanPaste();
    }



void ReviewPage::makeNotActive() {
    }


void ReviewPage::switchFields() {
    if( mReviewNameField.isFocused() ) {
        mReviewTextArea.focus();
        }
    else if( mReviewTextArea.isFocused() ) {
        mReviewNameField.focus();
        }
    }



void ReviewPage::keyDown( unsigned char inASCII ) {
    if( inASCII == 9 ) {
        // tab
        switchFields();
        return;
        }
    }
