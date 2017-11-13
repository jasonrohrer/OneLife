#include "ServerActionPage.h"

#include "TextButton.h"
#include "TextArea.h"
#include "TextField.h"
#include "RadioButtonSet.h"


#include "minorGems/ui/event/ActionListener.h"




class ReviewPage : public ServerActionPage, public ActionListener {
        
    public:
        
        ReviewPage( const char *inReviewServerURL );
        ~ReviewPage();
        

        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );

        virtual void step();

        virtual void actionPerformed( GUIComponent *inTarget );
        
        virtual void keyDown( unsigned char inASCII );

        
        virtual void makeActive( char inFresh );
        virtual void makeNotActive();

    protected:

        TextField mReviewNameField;
        
        RadioButtonSet *mRecommendChoice;

        TextArea mReviewTextArea;
        
        CheckboxButton mSpellcheckButton;

        TextButton mBackButton;

        
        TextButton mPostButton;
        TextButton mRemoveButton;
        

        TextButton mCopyButton;
        TextButton mPasteButton;

        TextButton mClearButton;
        
        char mGettingSequenceNumber;
        char mRemoving;
        

        void switchFields();

        void checkCanPost();
        void checkCanPaste();
        
        void saveReview();

    };
