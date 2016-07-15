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
        
        virtual void actionPerformed( GUIComponent *inTarget );

        
        virtual void makeActive( char inFresh );
        virtual void makeNotActive();

        virtual void step();
        

        // for TAB and ENTER (switch fields and start login)
        virtual void keyDown( unsigned char inASCII );
        
        // for arrow keys (switch fields)
        virtual void specialKeyDown( int inKeyCode );
        
    protected:
        
        TextField mEmailField;
        TextField mKeyField;

        TextField *mFields[2];

        TextButton mAtSignButton;

        KeyEquivalentTextButton mPasteButton;
        
        TextButton mLoginButton;
        TextButton mLoginNoSaveButton;
        TextButton mCancelButton;


        void switchFields();
        
        void processLogin( char inStore );

    };

