#include "ServerActionPage.h"

#include "TextButton.h"
#include "TextField.h"


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
        char *mCurrentLeaderEmail;
        
        char mPosting;
        


        TextField mGithubAccountNameField;
        
        TextField mContentLeaderVoteField;        

        TextButton mBackButton;

        
        TextButton mPostButton;
        
        char mGettingSequenceNumber;


        void switchFields();
        
        void saveSettings();
        
        void setupRequest( const char *inActionName );
        
    };
