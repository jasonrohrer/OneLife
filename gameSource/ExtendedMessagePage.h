#include "GamePage.h"



#include "minorGems/ui/event/ActionListener.h"


#include "TextButton.h"


class ExtendedMessagePage : public GamePage, public ActionListener {
        
    public:
        ExtendedMessagePage();
        

        void setMessageKey( const char *inMessageKey );
        

        virtual void actionPerformed( GUIComponent *inTarget );

        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );
        
    protected:
        
        TextButton mOKButton;

        const char *mMessageKey;


    };
