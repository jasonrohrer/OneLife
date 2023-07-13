#include "GamePage.h"

#include "TextField.h"
#include "TextButton.h"


#include "minorGems/ui/event/ActionListener.h"


class ServicesPage : public GamePage, public ActionListener {
        
    public:
        
        ServicesPage();
        
        virtual ~ServicesPage();
        
        virtual void actionPerformed( GUIComponent *inTarget );

        
        virtual void makeActive( char inFresh );
        
        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );

    protected:
        TextField mChallengeField;
        
        TextButton mCopyEmailButton;
        TextButton mViewEmailButton;
        
        TextButton mPasteButton;
        TextButton mCopyHashButton;

        TextButton mBackButton;
        
        char *mHashString;
        
        char mEmailVisible;
    };

