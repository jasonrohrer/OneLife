#include "GamePage.h"

#include "TextButton.h"
#include "TextArea.h"
#include "TextField.h"
#include "RadioButtonSet.h"


#include "minorGems/ui/event/ActionListener.h"




class ReviewPage : public GamePage, public ActionListener {
        
    public:
        
        ReviewPage();
        

        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );

        virtual void step();

        virtual void actionPerformed( GUIComponent *inTarget );
        
        virtual void keyDown( unsigned char inASCII );

        
        virtual void makeActive( char inFresh );
        virtual void makeNotActive();

    protected:

        TextField mReviewNameField;
        
        //RadioButtonSet mRecommendChoice;

        TextArea mReviewTextArea;
        
        TextButton mBackButton;

        
        //TextButton mSaveButton;
        //TextButton mPostButton;
        

        //TextButton mCopyButton;
        //TextButton mPasteButton;


        void switchFields();
        
        
    };
