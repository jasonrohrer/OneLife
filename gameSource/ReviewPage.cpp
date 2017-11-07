#include "ReviewPage.h"


#include "minorGems/game/Font.h"

#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/game/game.h"

#include "buttonStyle.h"


extern Font *mainFont;
extern Font *mainFontReview;


ReviewPage::ReviewPage()
        : mReviewNameField( mainFont, -242, 250, 10, false,
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
          mPostButton( mainFont, 526, -140, translate( "postReviewButton" ) ),
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



void ReviewPage::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mBackButton ) {
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
        }
    }



void ReviewPage::checkCanPaste() {
    if( mReviewTextArea.isActive() ) {
        
        char foc = mReviewTextArea.isFocused();
        
        mCopyButton.setVisible( foc );
        mPasteButton.setVisible( foc );
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

    
    checkCanPost();
    checkCanPaste();

    mCopyButton.setActive( true );
    mPasteButton.setActive( true );
    mClearButton.setActive( true );

    

    mReviewNameField.focus();
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
