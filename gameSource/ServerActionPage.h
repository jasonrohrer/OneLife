#ifndef SERVER_ACTION_PAGE_INCLUDED
#define SERVER_ACTION_PAGE_INCLUDED


#include "GamePage.h"

#include "TextButton.h"
#include "TextField.h"


#include "minorGems/system/Time.h"



// base class for pages that execute a server action and step
// to wait for and parse a response
// consolidates server contact and response error-handling code
class ServerActionPage : public GamePage {
    public:
        
        // inResponsePartNames copied internally, destroyed by caller
        ServerActionPage( const char *inServerURL,
                          const char *inActionName,
                          int inRequiredNumResponseParts,
                          const char *inResponsePartNames[],
                          char inAttachAccountHmac = true );
        

        // for responses with a variable number of parts
        ServerActionPage( const char *inServerURL,
                          const char *inActionName,
                          char inAttachAccountHmac = true );
        
                          
        
        virtual ~ServerActionPage();
        

        void setActionName( const char *inActionName );
        
        
        // clears old response part names
        // if inRequiredNumResponseParts is -1 
        // (and inResponsePartNames is NULL)
        // then response will be allowed to have a variable number of parts
        void setResponsePartNames( int inRequiredNumResponseParts,
                                   const char *inResponsePartNames[] );
        


        void setActionParameter( const char *inParameterName,
                                 const char *inParameterValue );
        
        void setActionParameter( const char *inParameterName,
                                 int inParameterValue );

        void setActionParameter( const char *inParameterName,
                                 double inParameterValue );
        
        
        void clearActionParameters();
        

        // defaults to true
        void setAttachAccountHmac( char inShouldAttach );


        // beyond DENIED, which is built-in and maps to key requestDenied, 
        // adds other error strings to be handled specially
        void addServerErrorString( const char *inServerErrorString,
                                   const char *inUserMessageKey );        

        // for more complicated error handling beyond displaying
        // a message on this page, can also set a named signal
        void addServerErrorStringSignal( const char *inServerErrorString,
                                         const char *inSignalToSet );


        // default behavior is to start request immediately upon
        // makeActive (this is great for single-action pages)
        // override makeActive to change this behavior
        virtual void makeActive( char inFresh );

        virtual void step();
        


        char isError();

        char isResponseReady();
        
        char isActionInProgress();
        

        // result destroyed by caller
        char *getResponse( const char *inPartName );
        
        int getResponseInt( const char *inPartName );
        
        double getResponseDouble( const char *inPartName );
        
        
        // for variable-length response lists
        int getNumResponseParts();
        
        char *getResponse( int inPartNumber );


        
        // defaults to 0
        // sets minimum time before response from server is processed
        // and flagged with isResponseReady
        // (To allow user to read messages during server action, like
        //  "Logging in...")
        void setMinimumResponseTime( unsigned int inSeconds );


        // call this before adding hmac-signed parameters below
        void setupRequestParameterSecurity();
        
        
        // for two-parameter, hmac-signed parameters
        void setParametersFromField( const char *inParamName,
                                     TextField *inField );
        
        void setParametersFromString( const char *inParamName,
                                      const char *inString );

    protected:
        
        void startRequest();

        
        char *mServerURL;
        char *mActionName;

        SimpleVector<char*> mActionParameterNames;
        SimpleVector<char*> mActionParameterValues;
        
        char mAttachAccountHmac;

        SimpleVector<char *> mErrorStringList;
        SimpleVector<const char *> mErrorStringUserMessageKeys;

        SimpleVector<char *> mErrorStringListForSignals;
        SimpleVector<const char *> mErrorStringSignals;
        

        int mNumResponseParts;
        
        SimpleVector<char*> mResponsePartNames;
        SimpleVector<char*> mResponseParts;
        

        int mWebRequest;
        char mResponseReady;


        int mMinimumResponseSeconds;
        
        timeSec_t mRequestStartTime;


        char *mParameterHmacKey;
        


    };

        

#endif
