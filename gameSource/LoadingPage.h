#include "GamePage.h"

#include "minorGems/ui/event/ActionListener.h"


class LoadingPage : public GamePage {
        
    public:
        

        void setCurrentPhase( const char *inPhaseName );
        
        void setCurrentProgress( float inProgress );
        

        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );
        

    private:
        const char *mPhaseName;
        float mProgress;
    };
