#include "GamePage.h"

#include "TextField.h"
#include "TextButton.h"
#include "KeyEquivalentTextButton.h"


#include "minorGems/ui/event/ActionListener.h"


class ExistingAccountPage : public GamePage, public ActionListener {
        
    public:
        
        ExistingAccountPage();
        
        virtual ~ExistingAccountPage();
        
        void clearFields();


        // defaults to true
        void showReviewButton( char inShow );
        
        // defaults to false
        void showDisableCustomServerButton( char inShow );
        

        
        virtual void actionPerformed( GUIComponent *inTarget );

        
        virtual void makeActive( char inFresh );
        virtual void makeNotActive();

        virtual void step();
        

        // for TAB and ENTER (switch fields and start login)
        virtual void keyDown( unsigned char inASCII );
        
        // for arrow keys (switch fields)
        virtual void specialKeyDown( int inKeyCode );
        
        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );


    protected:
        
        TextField mEmailField;
        TextField mKeyField;

        TextField *mFields[2];

        TextButton mAtSignButton;

        KeyEquivalentTextButton mPasteButton;

        TextButton mDisableCustomServerButton;
        
        TextButton mLoginButton;
        TextButton mFriendsButton;
        TextButton mGenesButton;
        TextButton mFamilyTreesButton;
        TextButton mClearAccountButton;
        TextButton mCancelButton;

        TextButton mSettingsButton;
        TextButton mReviewButton;
        
        TextButton mRetryButton;
        TextButton mRedetectButton;

        TextButton mViewAccountButton;
        
        TextButton mTutorialButton;
        

        double mPageActiveStartTime;
        int mFramesCounted;
        char mFPSMeasureDone;

        char mHideAccount;

        void switchFields();
        
        void processLogin( char inStore, const char *inSignal );

    };

