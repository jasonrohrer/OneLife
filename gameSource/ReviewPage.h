#include "GamePage.h"

#include "TextButton.h"
#include "TextArea.h"
#include "CheckboxButton.h"
#include "ValueSlider.h"
#include "SoundUsage.h"


#include "minorGems/ui/event/ActionListener.h"




class ReviewPage : public GamePage, public ActionListener {
        
    public:
        
        ReviewPage();
        

        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );

        virtual void step();

        virtual void actionPerformed( GUIComponent *inTarget );

        
        virtual void makeActive( char inFresh );
        virtual void makeNotActive();

    protected:

        TextArea mReviewTextArea;
        
        TextButton mBackButton;
        
    };
