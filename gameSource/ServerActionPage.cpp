#include "ServerActionPage.h"

#include "minorGems/game/game.h"

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"

#include "minorGems/util/stringUtils.h"

#include "minorGems/crypto/hashes/sha1.h"

#include "minorGems/network/web/URLUtils.h"


#include "serialWebRequests.h"
#include "accountHmac.h"

#include "minorGems/system/Time.h"



extern int userID;
extern int serverSequenceNumber;



ServerActionPage::ServerActionPage( const char *inServerURL,
                                    const char *inActionName,
                                    int inRequiredNumResponseParts,
                                    const char *inResponsePartNames[],
                                    char inAttachAccountHmac )
        : mServerURL( stringDuplicate( inServerURL ) ),
          mActionName( stringDuplicate( inActionName ) ),
          mAttachAccountHmac( inAttachAccountHmac ),
          mNumResponseParts( inRequiredNumResponseParts ),
          mWebRequest( -1 ), mResponseReady( false ),
          mMinimumResponseSeconds( 0 ),
          mRequestStartTime( 0 ),
          mParameterHmacKey( NULL ) {
    
    for( int i=0; i<mNumResponseParts; i++ ) {
        mResponsePartNames.push_back( 
            stringDuplicate( inResponsePartNames[i] ) );
        }

    addServerErrorString( "DENIED", "requestDenied" );
    }



ServerActionPage::ServerActionPage( const char *inServerURL,
                                    const char *inActionName,
                                    char inAttachAccountHmac )
        : mServerURL( stringDuplicate( inServerURL ) ),
          mActionName( stringDuplicate( inActionName ) ),
          mAttachAccountHmac( inAttachAccountHmac ),
          mNumResponseParts( -1 ),
          mWebRequest( -1 ), mResponseReady( false ),
          mMinimumResponseSeconds( 0 ),
          mRequestStartTime( 0 ),
          mParameterHmacKey( NULL ) {
    
    addServerErrorString( "DENIED", "requestDenied" );
    }



ServerActionPage::~ServerActionPage() {
    
        
    if( mWebRequest != -1 ) {
        clearWebRequestSerial( mWebRequest );
        }

    
    delete [] mServerURL;
    delete [] mActionName;

    mActionParameterNames.deallocateStringElements();
    mActionParameterValues.deallocateStringElements();
        
    mResponsePartNames.deallocateStringElements();
    mResponseParts.deallocateStringElements();

    
    for( int i=0; i<mErrorStringList.size(); i++ ) {
        delete [] *( mErrorStringList.getElement( i ) );
        }
    
    for( int i=0; i<mErrorStringListForSignals.size(); i++ ) {
        delete [] *( mErrorStringListForSignals.getElement( i ) );
        }
    
    if( mParameterHmacKey != NULL ) {
        delete [] mParameterHmacKey;
        mParameterHmacKey = NULL;
        }
    }



void ServerActionPage::setActionName( const char *inActionName ) {
    delete [] mActionName;
    mActionName = stringDuplicate( inActionName );
    }



void ServerActionPage::clearActionParameters() {
    mActionParameterNames.deallocateStringElements();
    mActionParameterValues.deallocateStringElements();
    }


void ServerActionPage::setResponsePartNames( 
    int inRequiredNumResponseParts,
    const char *inResponsePartNames[] ) {
    
    mResponsePartNames.deallocateStringElements();
    mResponseParts.deallocateStringElements();

    mNumResponseParts = inRequiredNumResponseParts;
    
    if( mNumResponseParts != -1 ) {
    
        for( int i=0; i<mNumResponseParts; i++ ) {
            mResponsePartNames.push_back( 
                stringDuplicate( inResponsePartNames[i] ) );
            }
        }
    }

        

void ServerActionPage::setActionParameter( const char *inParameterName,
                                           const char *inParameterValue ) {
    for( int i=0; i<mActionParameterNames.size(); i++ ) {
        char *name = *( mActionParameterNames.getElement( i ) );
    
        if( strcmp( name, inParameterName ) == 0 ) {
            mActionParameterNames.deallocateStringElement( i );
            mActionParameterValues.deallocateStringElement( i );
            }
        }
    
    mActionParameterNames.push_back( stringDuplicate( inParameterName ) );
    mActionParameterValues.push_back( stringDuplicate( inParameterValue ) );
    }


        
void ServerActionPage::setActionParameter( const char *inParameterName,
                                           int inParameterValue ) {
    char *valueString = autoSprintf( "%d", inParameterValue );
    setActionParameter( inParameterName, valueString );
    delete [] valueString;
    }



void ServerActionPage::setActionParameter( const char *inParameterName,
                                           double inParameterValue ) {
    
    char *valueString = autoSprintf( "%d", inParameterValue );
    setActionParameter( inParameterName, valueString );
    delete [] valueString;
    }



void ServerActionPage::setAttachAccountHmac( char inShouldAttach ) {
    mAttachAccountHmac = inShouldAttach;
    }



void ServerActionPage::addServerErrorString( const char *inServerErrorString,
                                             const char *inUserMessageKey ) {
    mErrorStringList.push_back( stringDuplicate( inServerErrorString ) );
    mErrorStringUserMessageKeys.push_back( inUserMessageKey );
    }



void ServerActionPage::addServerErrorStringSignal( 
    const char *inServerErrorString,
    const char *inSignalToSet ) {
    
    mErrorStringListForSignals.push_back( 
        stringDuplicate( inServerErrorString ) );
    mErrorStringSignals.push_back( inSignalToSet );
    }



void ServerActionPage::makeActive( char inFresh ) {
    if( !inFresh ) {
        return;
        }
    startRequest();
    }



void ServerActionPage::startRequest() {

    if( mWebRequest != -1 ) {
        clearWebRequest( mWebRequest );
        mWebRequest = -1;
        }
    
    mResponseParts.deallocateStringElements();


    
    SimpleVector<char> actionParameterListChars;
    
    for( int i=0; i<mActionParameterNames.size(); i++ ) {
        char *name = *( mActionParameterNames.getElement(i) );
        char *value = *( mActionParameterValues.getElement(i) );
        
        char *pairString = autoSprintf( "&%s=%s", name, value );
        
        actionParameterListChars.appendElementString( pairString );
        
        delete [] pairString;
        }
    

    if( mAttachAccountHmac ) {
        
        char *accountHmac = getAccountHmac();
        
        actionParameterListChars.appendElementString( accountHmac );
        
        delete [] accountHmac;
        }

    if( userID != -1 ) {
        char *userIDString = autoSprintf( "&user_id=%d", userID );
        
        actionParameterListChars.appendElementString( userIDString );
        
        delete [] userIDString;
        }
    
    
    char *actionParameterListString = 
        actionParameterListChars.getElementString();
    


    char *fullRequestURL = autoSprintf( 
        "%s?action=%s%s",
        mServerURL, mActionName, actionParameterListString );

    delete [] actionParameterListString;

    
    mWebRequest = startWebRequestSerial( "GET", 
                                         fullRequestURL, 
                                         NULL );
    
    delete [] fullRequestURL;
    

    mStatusError = false;
    mStatusMessageKey = NULL;

    mResponseReady = false;
    
    mRequestStartTime = game_timeSec();

    setWaiting( true );
    }

    

void ServerActionPage::step() {
    if( mWebRequest != -1 ) {
            
        int stepResult = stepWebRequestSerial( mWebRequest );
        
        
        if( mMinimumResponseSeconds > 0 &&
            game_timeSec() -  mRequestStartTime 
            < mMinimumResponseSeconds ) {
            
            // wait to process result
            return;
            }
        
        if( stepResult != 0 ) {
            setWaiting( false );
            }
        
        switch( stepResult ) {
            case 0:
                break;
            case -1:
                mStatusError = true;
                mStatusMessageKey = "err_webRequest";
                clearWebRequestSerial( mWebRequest );
                mWebRequest = -1;
                break;
            case 1: {
                char *result = getWebResultSerial( mWebRequest );
                clearWebRequestSerial( mWebRequest );
                mWebRequest = -1;
                     
                printf( "Web result = %s\n", result );
   
                char errorParsed = false;
                
                if( strcmp( result, "" ) == 0 ) {
                    mStatusError = true;
                    setStatus( "err_badServerResponse", true );
                    errorParsed = true;
                    }
                
                
                for( int i=0; i<mErrorStringList.size(); i++ ) {
                    if( strstr( result, 
                                *( mErrorStringList.getElement(i) ) )
                        != NULL ) {
                        
                        mStatusError = true;
                        mStatusMessageKey = 
                            *( mErrorStringUserMessageKeys.getElement(i) ); 
                        errorParsed = true;
                        break;
                        }
                    }
                for( int i=0; i<mErrorStringListForSignals.size(); i++ ) {
                    if( strstr( result, 
                                *( mErrorStringListForSignals.getElement(i) ) )
                        != NULL ) {
                        
                        setSignal( 
                            *( mErrorStringSignals.getElement(i) ) ); 
                        errorParsed = true;
                        break;
                        }
                    }
                    
                if( !errorParsed ) {
                    SimpleVector<char *> *lines = 
                        tokenizeString( result );
                    
                    if( mNumResponseParts != -1 
                        // fixed number response parts
                        &&
                        ( lines->size() != mNumResponseParts + 1
                          ||
                          strcmp( *( lines->getElement( mNumResponseParts ) ), 
                                  "OK" ) != 0 ) ) {

                        mStatusError = true;
                        setStatus( "err_badServerResponse", true );
                        

                        for( int i=0; i<lines->size(); i++ ) {
                            delete [] *( lines->getElement( i ) );
                            }
                        }
                    else if( mNumResponseParts == -1 
                             // variable number response parts
                             && 
                             strcmp( *( lines->getElement( lines->size()-1 ) ),
                                     "OK" ) != 0 ) {
                        mStatusError = true;
                        setStatus( "err_badServerResponse", true );
                        

                        for( int i=0; i<lines->size(); i++ ) {
                            delete [] *( lines->getElement( i ) );
                            }
                        }
                    else {
                        // all except final OK 
                        for( int i=0; i<lines->size()-1; i++ ) {
                            mResponseParts.push_back( 
                                *( lines->getElement( i ) ) );
                            }
                        
                        delete [] *( lines->getElement( lines->size()-1 ) );
                        
                        mResponseReady = true;
                        }
                    delete lines;
                    }
                        
                        
                delete [] result;
                }
                break;
            }
        }
    }




char ServerActionPage::isError() {
    return mStatusError;
    }



char ServerActionPage::isResponseReady() {
    return mResponseReady;
    }



char ServerActionPage::isActionInProgress() {
    return ( mWebRequest != -1 );
    }



// result destroyed by caller
char *ServerActionPage::getResponse( const char *inPartName ) {
    for( int i=0; i<mResponseParts.size(); i++ ) {
        char *name = *( mResponsePartNames.getElement( i ) );
        
        if( strcmp( name, inPartName ) == 0 ) {
            return getResponse( i );
            }
        }
    return NULL;
    }



int ServerActionPage::getResponseInt( const char *inPartName ) {
    char *responseString = getResponse( inPartName );
    
    if( responseString != NULL ) {
        int returnValue = -1;
        
        sscanf( responseString, "%d", &returnValue );

        delete [] responseString;
        
        return returnValue;
        }
    else {
        return -1;
        }
    }


double ServerActionPage::getResponseDouble( const char *inPartName ) {
    char *responseString = getResponse( inPartName );
    
    if( responseString != NULL ) {
        double returnValue = -1.0;
        
        sscanf( responseString, "%lf", &returnValue );

        delete [] responseString;
        
        return returnValue;
        }
    else {
        return -1.0;
        }
    }



int ServerActionPage::getNumResponseParts() {
    return mResponseParts.size();
    }



char *ServerActionPage::getResponse( int inPartNumber ) {
    return stringDuplicate( *( mResponseParts.getElement( inPartNumber ) ) );
    }




void ServerActionPage::setMinimumResponseTime( unsigned int inSeconds ) {
    mMinimumResponseSeconds = inSeconds;
    }



void ServerActionPage::setParametersFromField( const char *inParamName,
                                               TextField *inField ) {
    
    char *value = inField->getText();

    setParametersFromString( inParamName, value );
    
    delete [] value;
    }



void ServerActionPage::setParametersFromString( const char *inParamName,
                                               const char *inString ) {
    
    if( mParameterHmacKey == NULL ) {
        printf( "HmacKey not set before calling setParametersFromString\n" );
        return;
        }
    
    char *value_hmac = hmac_sha1( mParameterHmacKey, inString );
        
    char *encodedValue = URLUtils::urlEncode( (char *)inString );
    
    setActionParameter( inParamName, encodedValue );
    delete [] encodedValue;
        
    char *hmacParamName = autoSprintf( "%s_hmac", inParamName );

    setActionParameter( hmacParamName, value_hmac );
    delete [] hmacParamName;
    delete [] value_hmac;
    }




void ServerActionPage::setupRequestParameterSecurity() {    

    setActionParameter( "request_sequence_number", serverSequenceNumber );
        
    char *pureKey = getPureAccountKey();
    
    if( mParameterHmacKey != NULL ) {
        delete [] mParameterHmacKey;
        mParameterHmacKey = NULL;
        }
    
    mParameterHmacKey = autoSprintf( "%s%d", pureKey, serverSequenceNumber );
    delete [] pureKey;

    char *tagString = autoSprintf( "%.f", Time::timeSec() );
    char *request_tag = hmac_sha1( mParameterHmacKey, tagString );
    delete [] tagString;

    setActionParameter( "request_tag", request_tag );
    delete [] request_tag;
    }
