#include "GamePage.h"



#include "minorGems/ui/event/ActionListener.h"


#include "TextButton.h"
#include "RadioButtonSet.h"


class PollPage : public GamePage, public ActionListener {

    public:
        PollPage( const char *inReviewServerURL );
        ~PollPage();
        

        virtual void actionPerformed( GUIComponent *inTarget );

        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );

        virtual void makeActive( char inFresh );
        
        virtual void step();

    protected:
        char *mReviewServerURL;
        
        TextButton mSubmitButton;
        
        int mPollID;
        
        char *mQuestion;
        
        RadioButtonSet *mAnswerButtons;

        char mWebRequestIsSubmit;
        int mWebRequest;
        

        int measureSplitQuestion();
        

    };
