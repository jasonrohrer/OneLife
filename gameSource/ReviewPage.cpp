#include "ReviewPage.h"


#include "minorGems/game/Font.h"

#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/game/game.h"

#include "buttonStyle.h"


extern Font *mainFont;
extern Font *mainFontReview;


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
          mBackButton( mainFont, -526, -140, translate( "backButton" ) ),
          mPostButton( mainFont, 526, -140, translate( "postButton" ) ),
          mCopyButton( mainFont, -526, 140, translate( "copy" ) ),
          mPasteButton( mainFont, -526, 40, translate( "paste" ) ),
          mClearButton( mainFont, 526, 140, translate( "clear" ) ) {

    const char *choiceList[2] = { translate( "recommendYes" ),
                                  translate( "recommendNo" ) };
    
    mRecommendChoice = 
        new RadioButtonSet( mainFont, 396, 258,
                            2, choiceList,
                            false, 4 ),
    
    
    setButtonStyle( &mBackButton );
    setButtonStyle( &mPostButton );
    setButtonStyle( &mCopyButton );
    setButtonStyle( &mPasteButton );
    setButtonStyle( &mClearButton );
    

    addComponent( &mBackButton );
    addComponent( &mPostButton );

    addComponent( &mCopyButton );
    addComponent( &mPasteButton );
    addComponent( &mClearButton );

        

    mBackButton.addActionListener( this );
    mPostButton.addActionListener( this );
    mCopyButton.addActionListener( this );
    mPasteButton.addActionListener( this );
    mClearButton.addActionListener( this );
    
    // add name field after so we can hit return in name field
    // and advance to text area without sending a return key to the text area
    addComponent( &mReviewTextArea );    
    addComponent( &mReviewNameField );

    addComponent( mRecommendChoice );


    mReviewNameField.setMaxLength( 20 );
    mReviewTextArea.setMaxLength( 5000 );

    mReviewNameField.addActionListener( this );

    mReviewNameField.setLabelTop( true );


    mPostButton.setMouseOverTip( translate( "postReviewTip" ) );


    mCopyButton.setMouseOverTip( translate( "copyReviewTip" ) );


    if( ! isClipboardSupported() ) {
        mCopyButton.setVisible( false );
        mPasteButton.setVisible( false );
        }
    
    }



ReviewPage::~ReviewPage() {
    delete mRecommendChoice;
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
        setSignal( "back" );
        }
    else if( inTarget == &mReviewNameField ) {
        switchFields();
        }
    else if( inTarget == &mCopyButton ) {
        mReviewTextArea.focus();

        char *text = mReviewTextArea.getText();
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
    else if( inTarget == &mPostButton ) {
        saveReview();
        mCopyButton.setActive( false );
        mPasteButton.setActive( false );
        mClearButton.setActive( false );
        mReviewNameField.setActive( false );
        mReviewTextArea.setActive( false );
        mRecommendChoice->setActive( false );
        
        //mBackButton.setActive( false );

        mPostButton.setVisible( false );
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
        }
    }




void ReviewPage::step() {
    checkCanPost();
    checkCanPaste();
    }





void ReviewPage::makeActive( char inFresh ) {
    if( inFresh ) {        
        }


    mReviewNameField.setActive( true );
    mReviewTextArea.setActive( true );
    mRecommendChoice->setActive( true );

    

    mCopyButton.setActive( true );
    mPasteButton.setActive( true );
    mClearButton.setActive( true );

    

    mReviewNameField.focus();


    char *reviewName = SettingsManager::getStringSetting( "reviewName", "" );
    char *reviewText = SettingsManager::getStringSetting( "reviewText", "" );
    
    int reviewRecommend = 
        SettingsManager::getIntSetting( "reviewRecommend", 1 );
    
    if( reviewRecommend == 0 ) {    
        mRecommendChoice->setSelectedItem( 1 );
        }
    else {
        mRecommendChoice->setSelectedItem( 0 );
        }
    
    mReviewNameField.setText( reviewName );
    mReviewTextArea.setText( reviewText );

    delete [] reviewName;
    delete [] reviewText;
    

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
