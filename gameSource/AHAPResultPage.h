#include "GamePage.h"

#include "TextButton.h"
#include "TextField.h"


#include "minorGems/ui/event/ActionListener.h"


class AHAPResultPage : public GamePage, public ActionListener {
        
    public:
        
        AHAPResultPage();
        
        virtual ~AHAPResultPage();
        
        virtual void actionPerformed( GUIComponent *inTarget );

        
        virtual void makeActive( char inFresh );
        
        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );

    protected:
        TextButton mCopyAccountURLButton;
        
        TextField mSteamKeyField;
        TextButton mCopySteamKeyButton;
        

        TextButton mOKButton;
    };

