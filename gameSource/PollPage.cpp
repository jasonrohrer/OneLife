#include "PollPage.h"



#include "buttonStyle.h"

#include "message.h"
#include "accountHmac.h"


#include "minorGems/game/Font.h"

#include "minorGems/game/game.h"

#include "minorGems/network/web/URLUtils.h"
#include "minorGems/crypto/hashes/sha1.h"

#include "minorGems/util/stringUtils.h"



extern Font *mainFont;

extern char *userEmail;
extern char *accountKey;


PollPage::PollPage( const char *inReviewServerURL ) 
        : mReviewServerURL( stringDuplicate( inReviewServerURL ) ),
          mSubmitButton( mainFont, 526, -300, translate( "submit" ) ),
          mPollID( 0 ),
          mQuestion( NULL ),
          mAnswerButtons( NULL ),
          mWebRequestIsSubmit( false ),
          mWebRequest( -1 ) {

    setButtonStyle( &mSubmitButton );

    //mSubmitButton.setVisible( false );
    
    mSubmitButton.addActionListener( this );
    addComponent( &mSubmitButton );
    }


PollPage::~PollPage() {
    delete [] mReviewServerURL;


    if( mQuestion != NULL ) {
        delete [] mQuestion;
        }
    
    
    if( mWebRequest != -1 ) {
        clearWebRequest( mWebRequest );
        mWebRequest = -1;
        }

    if( mAnswerButtons != NULL ) {
        removeComponent( mAnswerButtons );
        delete mAnswerButtons;
        mAnswerButtons = NULL;
        }
    }

        

void PollPage::actionPerformed( GUIComponent *inTarget ) {

    if( inTarget == &mSubmitButton ) {
        int pick = mAnswerButtons->getSelectedItem();
        
        char *stringToHash = autoSprintf( "%dv%d",
                                          mPollID, pick );
        
        char *key = getPureAccountKey();
        char *hash = hmac_sha1( key, stringToHash );
        
        printf( "Hashing %s with key %s\n", stringToHash, key );
        

        delete [] stringToHash;
        delete [] key;

        
        char *url = autoSprintf( "%s?action=poll_vote"
                                 "&email=%s"
                                 "&poll_id=%d"
                                 "&vote_number=%d"
                                 "&hash_value=%s",
                                 mReviewServerURL,
                                 userEmail,
                                 mPollID,
                                 pick,
                                 hash );
        
        delete [] hash;
        

        mWebRequest = startWebRequest( "GET", url, NULL );
        
        delete [] url;
        
        mWebRequestIsSubmit = true;
    
        mSubmitButton.setVisible( false );
        setWaiting( true );
        
        mAnswerButtons->setActive( false );
        }
    }


void PollPage::draw( doublePair inViewCenter, 
                     double inViewSize ) {
    
    if( mQuestion != NULL ) {
        doublePair pos = { 0, 200 };
                           
        drawMessage( "poll", pos );
        
        pos.y -= 50;
        
        if( strstr( mQuestion, "##" ) != NULL ) {
            // multi-line
            
            pos.x = -450;
            
            TextAlignment prev = getMessageAlign();
            
            setMessageAlign( alignLeft );
            
            drawMessage( mQuestion, pos );
            
            setMessageAlign( prev );
            }
        else {
            // one line, centered
            drawMessage( mQuestion, pos );
            }
        }
    }


static int numSteps = 0;



void PollPage::makeActive( char inFresh ) {
    if( ! inFresh ) {
        return;
        }
    numSteps = 0;
    
    if( mWebRequest != -1 ) {
        clearWebRequest( mWebRequest );
        mWebRequest = -1;
        }
    
    if( strcmp( userEmail, "" ) == 0 ||
        strcmp( accountKey, "" ) == 0 ) {
        setSignal( "done" );
        return;
        }

    char *url = autoSprintf( "%s?action=check_for_poll&"
                             "email=%s",
                             mReviewServerURL, userEmail );
    
    mWebRequest = startWebRequest( "GET", url, NULL );
    
    delete [] url;

    mWebRequestIsSubmit = false;
    
    mSubmitButton.setVisible( false );
    setWaiting( true );

    if( mQuestion != NULL ) {
        delete [] mQuestion;
        mQuestion = NULL;
        }

    if( mAnswerButtons != NULL ) {
        removeComponent( mAnswerButtons );
        delete mAnswerButtons;
        mAnswerButtons = NULL;
        }
    }



static void deallocateStringArray( char **inArray, int inNum ) {
    for( int i=0; i< inNum; i++ ) {
        delete [] inArray[i];
        }
    delete [] inArray;
    }



// returns number of lines
int PollPage::measureSplitQuestion() {
    SimpleVector<char *> lines;
    
    SimpleVector<char> charsLeft;
    
    int numChars = strlen( mQuestion );
    
    for( int i=0; i<numChars; i++ ) {
        charsLeft.push_back( mQuestion[i] );
        }
    
    while( charsLeft.size() > 0 ) {
        
        SimpleVector<char> workingLine;
        double workingLen = 0;
        

        while( charsLeft.size() > 0 &&
               ( workingLen < 900 ||
                 charsLeft.getElementDirect( 0 ) != ' ' ) ) {
            
            workingLine.push_back( charsLeft.getElementDirect( 0 ) );
            charsLeft.deleteElement( 0 );
            
            char *workingString = workingLine.getElementString();
            
            workingLen = mainFont->measureString( workingString );
            delete [] workingString;
            }
        
        char *fullLine = workingLine.getElementString();
        char *trimLine = trimWhitespace( fullLine );
        
        lines.push_back( trimLine );

        delete [] fullLine;
        }

    int numLines = lines.size();
    
    SimpleVector<char> splitQuestion;

    for( int i=0; i<numLines; i++ ) {
        char *thisLine;
        
        if( i < numLines - 1 ) {
            thisLine = autoSprintf( "%s##", lines.getElementDirect( i ) );
            }
        else {
            thisLine = stringDuplicate( lines.getElementDirect( i ) );
            }
        splitQuestion.appendElementString( thisLine );
        delete [] thisLine;
        }
    
    lines.deallocateStringElements();

    delete [] mQuestion;
    
    mQuestion = splitQuestion.getElementString();
    return numLines;
    }
    



void PollPage::step() {
    numSteps++;
    
    if( mWebRequest == -1 ) {
        return;
        }
    
    int result = stepWebRequest( mWebRequest );

    if( result == 0 ) {
        return;
        }

    // done or error
    setWaiting( false );

    if( result == -1 ) {
        clearWebRequest( mWebRequest );
        mWebRequest = -1;
        setSignal( "done" );
        return;
        }
    
    
    // result ready

    char *text = getWebResult( mWebRequest );
    clearWebRequest( mWebRequest );
    mWebRequest = -1;
    

    if( ! mWebRequestIsSubmit ) {
        // checking for poll

        if( strstr( text, "DENIED" ) == text ) {
            delete [] text;
            setSignal( "done" );
            return;
            }
        int numLines;
        char **lines = split( text, "\n", &numLines );
        
        delete [] text;

        if( numLines < 4 ) {
            deallocateStringArray( lines, numLines );
            setSignal( "done" );
            return;
            }
        
        mPollID = 0;
        
        sscanf( lines[0], "%d", &mPollID );
        
        if( mQuestion != NULL ) {
            delete [] mQuestion;
            }
        
        mQuestion = stringDuplicate( lines[1] );
        
        SimpleVector<char*> answers;
        
        for( int i=2; i<numLines-1; i++ ) {
            answers.push_back( autoSprintf( " %s", lines[i] ) );
            }
        
        if( mAnswerButtons != NULL ) {
            removeComponent( mAnswerButtons );
            delete mAnswerButtons;
            mAnswerButtons = NULL;
            }
        
        char **array = answers.getElementArray();
        
        int questionLines = measureSplitQuestion();

        mAnswerButtons = new RadioButtonSet( mainFont, 
                                             -400, 
                                             100 - 30 * questionLines,
                                             answers.size(), 
                                             (const char **)array,
                                             true,
                                             4 );
        delete [] array;

        answers.deallocateStringElements();
        
        addComponent( mAnswerButtons );
        
        deallocateStringArray( lines, numLines );
        mSubmitButton.setVisible( true );
        }
    else {
        // a submit request
        // just step until done, then end
        delete [] text;
        
        setSignal( "done" );
        }
    

    }

