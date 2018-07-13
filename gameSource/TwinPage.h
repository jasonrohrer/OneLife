#include "GamePage.h"

#include "TextField.h"
#include "TextButton.h"
#include "RadioButtonSet.h"


#include "minorGems/ui/event/ActionListener.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/random/JenkinsRandomSource.h"


class TwinPage : public GamePage, public ActionListener {
        
    public:
        
        TwinPage();
        
        virtual ~TwinPage();
        
        virtual void actionPerformed( GUIComponent *inTarget );

        
        virtual void makeActive( char inFresh );
        
        virtual void draw( doublePair inViewCenter, 
                           double inViewSize );

    protected:
        JenkinsRandomSource mRandSource;
        
        TextField mCodeField;

        TextButton mGenerateButton;

        TextButton mCopyButton;
        TextButton mPasteButton;
        
        TextButton mLoginButton;
        
        TextButton mCancelButton;
        
        RadioButtonSet *mPlayerCountRadioButtonSet;
        
        SimpleVector<char*> mWordList;
        
        
    };

