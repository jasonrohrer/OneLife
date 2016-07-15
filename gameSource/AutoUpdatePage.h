#include "GamePage.h"

#include "minorGems/ui/event/ActionListener.h"


class AutoUpdatePage : public GamePage {
        
    public:
        
        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );
        
        virtual void step();
                
    };

