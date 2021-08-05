#include "GamePage.h"

#include "TextField.h"
#include "TextButton.h"
#include "KeyEquivalentTextButton.h"
#include "DropdownList.h"

#include "minorGems/ui/event/ActionListener.h"
#include "PageComponent.h"



class Background : public PageComponent, public ActionListenerList {
        
    public:
        
        // text field width based on widest allowed 
        // (or non-forbidden) character

        // label text and char maps copied internally
        Background( const char *inImageName, float inOpacity = 1.0f, doublePair inPosition = {0, 0} );
        
        
        
        virtual void draw();
        

        
    protected:
        SpriteHandle mImage;

        float mOpacity;

        doublePair mPosition;
        
    };


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

        DropdownList mSpawnSeed;
        TextField *mFields[2];

        TextButton mAtSignButton;

        KeyEquivalentTextButton mPasteButton;

        TextButton mDisableCustomServerButton;
        
        Background mBackground;
        Background mGameLogo;
        
        TextButton mSeedButton;
        TextButton mUnlockButton;
        
        TextButton mLoginButton;
        TextButton mFriendsButton;
        TextButton mGenesButton;
        TextButton mFamilyTreesButton;
        TextButton mTechTreeButton;
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

