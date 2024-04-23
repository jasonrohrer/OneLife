#include "ServerActionPage.h"

#include "TextButton.h"
#include "TextField.h"
#include "KeyEquivalentTextButton.h"


#include "minorGems/ui/event/ActionListener.h"




class AHAPSettingsPage : public ServerActionPage, public ActionListener {
        
    public:
        
        AHAPSettingsPage( const char *inAHAPGateServerURL );
        ~AHAPSettingsPage();
        

        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );

        virtual void step();

        virtual void actionPerformed( GUIComponent *inTarget );
        
        virtual void keyDown( unsigned char inASCII );

        
        virtual void makeActive( char inFresh );
        virtual void makeNotActive();

    protected:

        int mSequenceNumber;
        char *mCurrentLeaderGithub;
        
        char mPosting;
        


        TextField mGithubAccountNameField;
        
        TextField mContentLeaderVoteField;        

        KeyEquivalentTextButton mPasteGithubButton;
        KeyEquivalentTextButton mPasteLeaderButton;

        TextButton mBackButton;

        
        TextButton mPostButton;
        
        char mGettingSequenceNumber;


        void switchFields();
        
        void saveSettings();
        
        // inExtraHmacConcat is concatonated after sequence number
        // when making HMAC
        void setupRequest( const char *inActionName,
                           const char *inExtraHmacConcat = "" );
        
        void testPostVisible();
    };
