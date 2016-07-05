#include "GamePage.h"

#include "minorGems/ui/event/ActionListener.h"


class LoadingPage : public GamePage {
        
    public:
        

        LoadingPage() 
                : mShowProgress( true ) {
            }
        

        void setCurrentPhase( const char *inPhaseName );
        
        void setCurrentProgress( float inProgress );
        
        // on by default
        void showProgress( char inShow ) {
            mShowProgress = inShow;
            }
        

        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );
        

    private:
        const char *mPhaseName;
        float mProgress;

        char mShowProgress;
    };
