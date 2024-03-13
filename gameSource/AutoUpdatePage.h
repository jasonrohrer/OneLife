#include "GamePage.h"

#include "minorGems/ui/event/ActionListener.h"


class AutoUpdatePage : public GamePage {
        
    public:
        
        AutoUpdatePage();
        

        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );
        
        virtual void step();
        

        // defaults to false
        void setUseAHAPMessaging( char inAHAP );
        
        // defaults to true
        // set to false to disable re-launch
        void setAutoRelaunch( char inRelaunch );
        
        
    protected:
        
        char mUseAHAPMessaging;
        char mAutoRelaunch;
        
    };

