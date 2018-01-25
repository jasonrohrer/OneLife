#include "EditorTransitionPage.h"

#include "minorGems/game/game.h"
#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"

#include "minorGems/graphics/converters/PNGImageConverter.h"

#include "minorGems/io/file/FileInputStream.h"



#include "message.h"


// label distance from checkboxes
static int checkboxSep = 12;



static int getObjectByIndex( TransRecord *inRecord, int inIndex ) {
    
    switch( inIndex ) {
        case 0:
            return inRecord->actor;
        case 1:
            return inRecord->target;
        case 2:
            return inRecord->newActor;
        case 3:
            return inRecord->newTarget;
        default:
            return -1;
        }
    }


static void setObjectByIndex( TransRecord *inRecord, int inIndex, int inID ) {
    
    switch( inIndex ) {
        case 0:
            inRecord->actor = inID;
            break;
        case 1:
            inRecord->target = inID;
            break;
        case 2:
            inRecord->newActor = inID;
            break;
        case 3:
            inRecord->newTarget = inID;
            break;
        }
    }



extern Font *mainFont;
extern Font *smallFont;



#include "ObjectPickable.h"

static ObjectPickable objectPickable;


#define NUM_MOVE_BUTTONS 8
static const char *moveButtonNames[NUM_MOVE_BUTTONS] =
{ "None", "Chase", "Flee", "Random", "North", "South", "East", "West" };


EditorTransitionPage::EditorTransitionPage()
        : mAutoDecayTimeField( smallFont, 
                               0,  -170, 6,
                               false,
                               "AutoDecay Seconds", "-0123456789", NULL ),
          mLastUseActorCheckbox( -330, 75, 2 ),
          mLastUseTargetCheckbox( 130, 75, 2 ),
          mReverseUseActorCheckbox( -330, -75, 2 ),
          mReverseUseTargetCheckbox( 130, -75, 2 ),
          mMovementButtons( smallFont, 240, 40,
                            NUM_MOVE_BUTTONS, moveButtonNames, true, 2 ),
          mDesiredMoveDistField( smallFont,
                                 240, 40 - NUM_MOVE_BUTTONS * 20 - 30, 4,
                                 false,
                                 "Dist", "0123456789", NULL ),
          mActorMinUseFractionField( smallFont, 
                                     -290,  115, 4,
                                     false,
                                     "Min Use Fraction", "0123456789.", NULL ),
          mTargetMinUseFractionField( smallFont, 
                                      90,  115, 4,
                                      false,
                                      "Min Use Fraction", "0123456789.", NULL ),
          mSaveTransitionButton( mainFont, -320, 0, "Save" ),
          mObjectPicker( &objectPickable, +410, 90 ),
          mObjectEditorButton( mainFont, 410, 260, "Objects" ),
          mCategoryEditorButton( mainFont, -410, 260, "Categories" ),
          mProducedByNext( smallFont, 180, 260, "Next" ),
          mProducedByPrev( smallFont, -180, 260, "Prev" ),
          mProducesNext( smallFont, 180, -260, "Next" ),
          mProducesPrev( smallFont, -180, -260, "Prev" ),
          mDelButton( smallFont, +150, 0, "Delete" ),
          mDelConfirmButton( smallFont, +150, 40, "Delete?" ),
          mSwapActorButton( "swapButton.tga", -200, 0 ),
          mSwapTargetButton( "swapButton.tga", 0, 0 ),
          mSwapTopButton( "swapButtonH.tga", -100, 120 ),
          mSwapBottomButton( "swapButtonH.tga", -100, -120 ) {

    mSwapActorButton.setPixelSize( 2 );
    mSwapTargetButton.setPixelSize( 2 );

    mSwapTopButton.setPixelSize( 2 );
    mSwapBottomButton.setPixelSize( 2 );

    addComponent( &mAutoDecayTimeField );
    mAutoDecayTimeField.setVisible( false );
    
    addComponent( &mLastUseActorCheckbox );
    mLastUseActorCheckbox.addActionListener( this );

    addComponent( &mLastUseTargetCheckbox );
    mLastUseTargetCheckbox.addActionListener( this );

    addComponent( &mReverseUseActorCheckbox );
    mReverseUseActorCheckbox.addActionListener( this );

    addComponent( &mReverseUseTargetCheckbox );
    mReverseUseTargetCheckbox.addActionListener( this );

    addComponent( &mMovementButtons );
    mMovementButtons.addActionListener( this );
    
    addComponent( &mDesiredMoveDistField );

    mActorMinUseFractionField.setFloat( 0, -1, true );
    mTargetMinUseFractionField.setFloat( 0, -1, true );
    
    mTargetMinUseFractionField.setLabelSide( true );

    addComponent( &mActorMinUseFractionField );
    addComponent( &mTargetMinUseFractionField );

    addComponent( &mSaveTransitionButton );
    addComponent( &mObjectPicker );
    addComponent( &mObjectEditorButton );
    addComponent( &mCategoryEditorButton );
    addComponent( &mDelButton );
    addComponent( &mDelConfirmButton );

    addComponent( &mProducedByNext );
    addComponent( &mProducedByPrev );
    addComponent( &mProducesNext );
    addComponent( &mProducesPrev );

    addComponent( &mSwapActorButton );
    addComponent( &mSwapTargetButton );
    
    addComponent( &mSwapTopButton );
    addComponent( &mSwapBottomButton );
    

    mSaveTransitionButton.addActionListener( this );
    mObjectEditorButton.addActionListener( this );
    mCategoryEditorButton.addActionListener( this );
    
    mObjectPicker.addActionListener( this );

    mSaveTransitionButton.setVisible( false );
    
    mDelButton.addActionListener( this );
    mDelConfirmButton.addActionListener( this );

    mSwapActorButton.addActionListener( this );
    mSwapTargetButton.addActionListener( this );

    mSwapTopButton.addActionListener( this );
    mSwapBottomButton.addActionListener( this );

    mDelButton.setVisible( false );
    mDelConfirmButton.setVisible( false );
    

    mProducedByNext.addActionListener( this );
    mProducedByPrev.addActionListener( this );
    mProducesNext.addActionListener( this );
    mProducesPrev.addActionListener( this );
    
    mProducedByNext.setVisible( false );
    mProducedByPrev.setVisible( false );
    mProducesNext.setVisible( false );
    mProducesPrev.setVisible( false );
    

    mCurrentTransition.actor = 0;
    mCurrentTransition.target = 0;
    mCurrentTransition.newActor = 0;
    mCurrentTransition.newTarget = 0;
    mCurrentTransition.autoDecaySeconds = 0;
    mCurrentTransition.lastUseActor = false;
    mCurrentTransition.lastUseTarget = false;
    mCurrentTransition.reverseUseActor = false;
    mCurrentTransition.reverseUseTarget = false;
    mCurrentTransition.actorMinUseFraction = 0;
    mCurrentTransition.targetMinUseFraction = 0;
    
    mCurrentTransition.move = 0;
    mCurrentTransition.desiredMoveDist = 1;

    mCurrentlyReplacing = 0;
    

    
    int yP = 75;
    
    int i = 0;
    
    for( int y = 0; y<2; y++ ) {
        int xP = - 200;
        for( int x = 0; x<2; x++ ) {
        
            // buttons hiding under displays to register clicks
            mPickButtons[i] = new Button( xP, yP, 100, 100, 1 );
            
            addComponent( mPickButtons[i] );
            
            mPickButtons[i]->addActionListener( this );
            
            int offset = -90;
            if( i == 1 || i == 3 ) {
                offset = 90;
                }
            
            mClearButtons[i] = 
                new TextButton( smallFont, xP + offset, yP, "X" );
            
            addComponent( mClearButtons[i] );
            mClearButtons[i]->addActionListener( this );            
            
            i++;
            
            xP += 200;
            }
        yP -= 150;
        }


    int xP = 0;
    for( int i=0; i<NUM_TREE_TRANS_TO_SHOW; i++ ) {
        mProducedByButtons[i] = new Button( xP, 260, 250, 100, 1 );
        mProducesButtons[i] = new Button( xP, -260, 250, 100, 1 );
        
        addComponent( mProducedByButtons[i] );
        addComponent( mProducesButtons[i] );
        
        mProducedByButtons[i]->addActionListener( this );
        mProducesButtons[i]->addActionListener( this );

        mProducedByButtons[i]->setVisible( false );
        mProducesButtons[i]->setVisible( false );
        
        xP += 150;

        for( int j=0; j<4; j++ ) {
            setObjectByIndex( &( mProducedBy[i] ), j, 0 );
            setObjectByIndex( &( mProduces[i] ), j, 0 );
            }
        }

    mProducedBySkip = 0;
    mProducesSkip = 0;
    

    mLastSearchID = -1;
    }



EditorTransitionPage::~EditorTransitionPage() {
    for( int i=0; i<4; i++ ) {
        delete mPickButtons[i];
        }
    for( int i=0; i<4; i++ ) {
        delete mClearButtons[i];
        }
    
    for( int i=0; i<NUM_TREE_TRANS_TO_SHOW; i++ ) {
        delete mProducedByButtons[i];
        delete mProducesButtons[i];
        }
    }





void EditorTransitionPage::clearUseOfObject( int inObjectID ) {
    for( int i=0; i<4; i++ ) {    
        if( getObjectByIndex( &mCurrentTransition, i ) == inObjectID ) {
            
            setObjectByIndex( &mCurrentTransition, i, 0 );
            }
        }
    checkIfSaveVisible();
    }




void EditorTransitionPage::checkIfSaveVisible() {
    char saveVis = ( mCurrentTransition.target != 0
                     ||
                     // food-eating transition
                     ( mCurrentTransition.actor > 0
                       &&
                       getObject( mCurrentTransition.actor )->foodValue > 0
                       &&
                       mCurrentTransition.newActor != 0 
                       &&
                       mCurrentTransition.newTarget == 0 )
                     ||
                     // generic use transition
                     ( mCurrentTransition.actor > 0
                       &&
                       getObject( mCurrentTransition.actor )->foodValue == 0
                       &&
                       mCurrentTransition.newActor != 0 
                       &&
                       mCurrentTransition.newTarget == 0 )
                     ||
                     // Use on bare ground transition
                     ( mCurrentTransition.actor > 0
                       &&
                       getObject( mCurrentTransition.actor )->foodValue == 0
                       &&
                       mCurrentTransition.newTarget != 0 ) );
    
    mSaveTransitionButton.setVisible( saveVis );
    

    char delVis = saveVis ||
        ( mCurrentTransition.actor > 0
          &&
          mCurrentTransition.newActor != 0 );

    mDelButton.setVisible( delVis );
    mDelConfirmButton.setVisible( false );

    mLastUseActorCheckbox.setToggled( mCurrentTransition.lastUseActor );
    mLastUseTargetCheckbox.setToggled( mCurrentTransition.lastUseTarget );
    
    mReverseUseActorCheckbox.setToggled( mCurrentTransition.reverseUseActor );
    mReverseUseTargetCheckbox.setToggled( mCurrentTransition.reverseUseTarget );

    mActorMinUseFractionField.setFloat( 
        mCurrentTransition.actorMinUseFraction, -1, true );
    
    mTargetMinUseFractionField.setFloat( 
        mCurrentTransition.targetMinUseFraction, -1, true );
    

    if( saveVis && 
        getObjectByIndex( &mCurrentTransition, 0 ) <= 0 &&
        getObjectByIndex( &mCurrentTransition, 2 ) == 0 ) {
        // no actor, no new actor, but there's a target

        // can be auto-decay
        mAutoDecayTimeField.setVisible( true );

        mMovementButtons.setVisible( true );
        mMovementButtons.setSelectedItem( mCurrentTransition.move );
        
        if( mCurrentTransition.move > 0 ) {
            mDesiredMoveDistField.setVisible( true );
            mDesiredMoveDistField.setInt( mCurrentTransition.desiredMoveDist );
            }
        else {
            mDesiredMoveDistField.setVisible( false );
            }

        if( mCurrentTransition.autoDecaySeconds != 0 ) {
            char *decayString =
                autoSprintf( "%d", mCurrentTransition.autoDecaySeconds );
            
            mAutoDecayTimeField.setText( decayString );
            
            delete [] decayString;
            }
        else {
            mAutoDecayTimeField.setText( "" );
            }
        }
    else {
        mAutoDecayTimeField.setVisible( false );
        mAutoDecayTimeField.setText( "" );

        mMovementButtons.setSelectedItem( 0 );
        mMovementButtons.setVisible( false );
        
        mDesiredMoveDistField.setInt( 1 );
        mDesiredMoveDistField.setVisible( false );    
        }
        
    }


static void fillInGenericPersonTarget( TransRecord *inRecord ) {
    if( inRecord->target == 0 ) {
        inRecord->target = getRandomPersonObject();
        
        if( inRecord->target == -1 ) {
            // no people in object bank?
            inRecord->target = 0;
            }
        }
    }


static void fillInGenericPersonActor( TransRecord *inRecord ) {
    if( inRecord->actor == -2 ) {
        inRecord->actor = getRandomPersonObject();
        
        if( inRecord->actor == -1 ) {
            // no people in object bank?
            inRecord->actor = 0;
            }
        }
    }



void EditorTransitionPage::redoTransSearches( int inObjectID,
                                              char inClearSkip ) {
    
    mLastSearchID = inObjectID;
    
    if( inClearSkip ) {
        mProducedBySkip = 0;
        mProducesSkip = 0;
        }

    printf( "Searching for %d, skips = %d,%d\n", 
            inObjectID,
            mProducedBySkip, mProducesSkip );


    int numResults, numLeft;
    TransRecord **resultsA = searchProduces( inObjectID, 
                                             mProducedBySkip, 
                                             NUM_TREE_TRANS_TO_SHOW, 
                                             &numResults, &numLeft );

    for( int i=0; i< NUM_TREE_TRANS_TO_SHOW; i++ ) {
        mProducedByButtons[i]->setVisible( false );
        for( int j=0; j<4; j++ ) {
            setObjectByIndex( &( mProducedBy[i] ), j, 0 );
            }
        }
    
    if( resultsA != NULL ) {
        

        for( int i=0; i<numResults; i++ ) {
            mProducedBy[i] = *( resultsA[i] );
            
            fillInGenericPersonTarget( &( mProducedBy[i] ) );
            fillInGenericPersonActor(  &( mProducedBy[i] ) );

            if( mProducedBy[i].target == -1 ) {
                mProducedBy[i].target = 0;
                }
            mProducedByButtons[i]->setVisible( true );
            }
    
    
        mProducedByPrev.setVisible( mProducedBySkip > 0 );
    
        mProducedByNext.setVisible( numLeft > 0 );

        delete [] resultsA;
        }
    else {
        mProducedByPrev.setVisible( false );
    
        mProducedByNext.setVisible( false );
        }
    

    
    TransRecord **resultsB = searchUses( inObjectID, 
                                         mProducesSkip, 
                                         NUM_TREE_TRANS_TO_SHOW, 
                                         &numResults, &numLeft );

    for( int i=0; i< NUM_TREE_TRANS_TO_SHOW; i++ ) {
        mProducesButtons[i]->setVisible( false );
        for( int j=0; j<4; j++ ) {
            setObjectByIndex( &( mProduces[i] ), j, 0 );
            }
        }


    if( resultsB != NULL ) {
        
        for( int i=0; i<numResults; i++ ) {
            mProduces[i] = *( resultsB[i] );
            
            fillInGenericPersonTarget( &( mProduces[i] ) );
            fillInGenericPersonActor( &( mProduces[i] ) );
            
            if( mProducedBy[i].target == -1 ) {
                mProducedBy[i].target = 0;
                }

            mProducesButtons[i]->setVisible( true );
            }
        
        mProducesPrev.setVisible( mProducesSkip > 0 );
        
        mProducesNext.setVisible( numLeft > 0 );
        
        delete [] resultsB;
        }
    else {
        mProducesPrev.setVisible( false );
    
        mProducesNext.setVisible( false );
        }

    
    }





void EditorTransitionPage::actionPerformed( GUIComponent *inTarget ) {
    
    if( inTarget == &mSaveTransitionButton ) {
        
        int decayTime = 0;
        char *decayText = mAutoDecayTimeField.getText();
        
        if( strcmp( decayText, "" ) != 0 ) {    
            sscanf( decayText, "%d", &decayTime );
            }            
        delete [] decayText;

        int actor = mCurrentTransition.actor;
        
        if( decayTime != 0 ) {
            // signal auto-delay
            actor = -1;
            }
        else if( actor == -1 ) {
            // correct it back to 0 for non-autoDecay transitions
            actor = 0;
            }
        
        int target = mCurrentTransition.target;
        
        if( target != 0 &&
            getObject( mCurrentTransition.target )->person ) {
            // all transitions with person target define what happens
            // when this actor is used on ANY person target
            // (this specific person object is a placeholder)
            
            // use target 0 to inidicate this
            target = 0;
            }
        else if( target == 0 &&
                 mCurrentTransition.actor > 0 &&
                 mCurrentTransition.newActor != 0 &&
                 mCurrentTransition.newTarget == 0 &&
                 getObject( mCurrentTransition.actor )->foodValue > 0 ) {
            // food eaten transition
            target = -1;
            }
        else if( target == 0 &&
                 mCurrentTransition.actor > 0 &&
                 mCurrentTransition.newActor != 0 &&
                 mCurrentTransition.newTarget == 0 &&
                 getObject( mCurrentTransition.actor )->foodValue == 0 ) {
            // generic use transition
            target = -1;
            }
        else if( target == 0 &&
                 mCurrentTransition.actor > 0 &&
                 mCurrentTransition.newTarget != 0 &&
                 getObject( mCurrentTransition.actor )->foodValue == 0 ) {
            
            target = -1;
            }
        else if( target != 0 &&
                 mCurrentTransition.newActor == 0 &&
                 mCurrentTransition.actor > 0 &&
                 getObject( mCurrentTransition.actor )->person ) {
            // default transition
            actor = -2;
            }
        
        addTrans( actor,
                  target,
                  mCurrentTransition.newActor,
                  mCurrentTransition.newTarget,
                  mCurrentTransition.lastUseActor,
                  mCurrentTransition.lastUseTarget,
                  mCurrentTransition.reverseUseActor,
                  mCurrentTransition.reverseUseTarget,
                  decayTime,
                  mActorMinUseFractionField.getFloat(),
                  mTargetMinUseFractionField.getFloat(),
                  mMovementButtons.getSelectedItem(),
                  mDesiredMoveDistField.getInt() );
            
        redoTransSearches( mLastSearchID, true );
        }
    if( inTarget == &mDelButton ) {
        mDelButton.setVisible( false );
        mDelConfirmButton.setVisible( true );
        }
    else if( inTarget == &mDelConfirmButton ) {

        int decayTime = 0;
        char *decayText = mAutoDecayTimeField.getText();
        
        if( strcmp( decayText, "" ) != 0 ) {    
            sscanf( decayText, "%d", &decayTime );
            }            
        delete [] decayText;

        int actor = mCurrentTransition.actor;
        
        if( decayTime != 0 ) {
            // signal auto-delay
            actor = -1;
            }
        else if( actor == -1 ) {
            // correct it back to 0 for non-autoDecay transitions
            actor = 0;
            }

        int target = mCurrentTransition.target;
        
        if( target != 0 &&
            getObject( mCurrentTransition.target )->person ) {
            // all transitions with person target define what happens
            // when this actor is used on ANY person target
            // (this specific person object is a placeholder)
            
            // use target 0 to inidicate this
            target = 0;
            }
        else if( target == 0 &&
                 mCurrentTransition.actor > 0 ) {
            // don't check that actor is food when deleting
            // (need to allow eat transition to be deleted after food
            // status removed from actor object)
            target = -1;
            }
        else if( target != 0 &&
                 mCurrentTransition.newActor == 0 &&
                 mCurrentTransition.actor > 0 &&
                 getObject( mCurrentTransition.actor )->person ) {
            // default transition
            actor = -2;
            }

        deleteTransFromBank( actor, target, 
                             mLastUseActorCheckbox.getToggled(),
                             mLastUseTargetCheckbox.getToggled() );
                             
        for( int i=0; i<4; i++ ) {
            setObjectByIndex( &mCurrentTransition, i, 0 );
            }
        mCurrentTransition.autoDecaySeconds = 0;
        
        checkIfSaveVisible();

        redoTransSearches( 0, true );
        }
    else if( inTarget == &mObjectPicker ) {
        if( mCurrentlyReplacing != -1 ) {
            
            int objectID = mObjectPicker.getSelectedObject();
            
            if( objectID != -1 ) {
                setObjectByIndex( &mCurrentTransition, mCurrentlyReplacing,
                                  objectID );
                
                checkIfSaveVisible();

                redoTransSearches( objectID, true );
                }
            }
        
        }
    else if( inTarget == &mLastUseActorCheckbox ) {
        mCurrentTransition.lastUseActor = mLastUseActorCheckbox.getToggled();
        }
    else if( inTarget == &mLastUseTargetCheckbox ) {
        mCurrentTransition.lastUseTarget = mLastUseTargetCheckbox.getToggled();
        }
    else if( inTarget == &mReverseUseActorCheckbox ) {
        mCurrentTransition.reverseUseActor = 
            mReverseUseActorCheckbox.getToggled();
        }
    else if( inTarget == &mReverseUseTargetCheckbox ) {
        mCurrentTransition.reverseUseTarget = 
            mReverseUseTargetCheckbox.getToggled();
        }
    else if( inTarget == &mMovementButtons ) {
        mCurrentTransition.move = mMovementButtons.getSelectedItem();
        
        if( mCurrentTransition.move > 0 ) {
            mDesiredMoveDistField.setVisible( true );
            }
        else {
            mDesiredMoveDistField.setInt( 1 );
            mDesiredMoveDistField.setVisible( false );
            }
        }
    else if( inTarget == &mSwapActorButton ) {
        
        int temp = mCurrentTransition.actor;
        
        mCurrentTransition.actor = mCurrentTransition.newActor;
        mCurrentTransition.newActor = temp;

        if( mCurrentlyReplacing == 0 ) {
            mCurrentlyReplacing = 2;
            }
        else if( mCurrentlyReplacing == 2 ) {
            mCurrentlyReplacing = 0;
            }
        
        checkIfSaveVisible();
        }
    else if( inTarget == &mSwapTargetButton ) {
        
        int temp = mCurrentTransition.target;
        
        mCurrentTransition.target = mCurrentTransition.newTarget;
        mCurrentTransition.newTarget = temp;
        
        if( mCurrentlyReplacing == 1 ) {
            mCurrentlyReplacing = 3;
            }
        else if( mCurrentlyReplacing == 3 ) {
            mCurrentlyReplacing = 1;
            }

        checkIfSaveVisible();
        }
    else if( inTarget == &mSwapTopButton ) {
        
        int temp = mCurrentTransition.actor;
        
        mCurrentTransition.actor = mCurrentTransition.target;
        mCurrentTransition.target = temp;

        if( mCurrentTransition.target == -1 ) {
            mCurrentTransition.target = 0;
            }

        if( mCurrentlyReplacing == 0 ) {
            mCurrentlyReplacing = 1;
            }
        else if( mCurrentlyReplacing == 1 ) {
            mCurrentlyReplacing = 0;
            }
        
        checkIfSaveVisible();
        }
    else if( inTarget == &mSwapBottomButton ) {
        
        int temp = mCurrentTransition.newTarget;
        
        mCurrentTransition.newTarget = mCurrentTransition.newActor;
        mCurrentTransition.newActor = temp;
        
        if( mCurrentlyReplacing == 2 ) {
            mCurrentlyReplacing = 3;
            }
        else if( mCurrentlyReplacing == 3 ) {
            mCurrentlyReplacing = 2;
            }

        checkIfSaveVisible();
        }
    else if( inTarget == &mObjectEditorButton ) {
        setSignal( "objectEditor" );
        }
    else if( inTarget == &mCategoryEditorButton ) {
        setSignal( "categoryEditor" );
        }
    else if( inTarget == &mProducedByNext ) {
        mProducedBySkip += NUM_TREE_TRANS_TO_SHOW;
        
        redoTransSearches( mLastSearchID, false );
        }
    else if( inTarget == &mProducedByPrev ) {
        mProducedBySkip -= NUM_TREE_TRANS_TO_SHOW;
        redoTransSearches( mLastSearchID, false );
        }
    else if( inTarget == &mProducesNext ) {
        mProducesSkip += NUM_TREE_TRANS_TO_SHOW;
        redoTransSearches( mLastSearchID, false );
        }
    else if( inTarget == &mProducesPrev ) {
        mProducesSkip -= NUM_TREE_TRANS_TO_SHOW;
        redoTransSearches( mLastSearchID, false );
        }    
    else {

        for( int i=0; i<4; i++ ) {
            if( inTarget == mPickButtons[i] ) {
                
                mCurrentlyReplacing = i;
                
                mObjectPicker.unselectObject();

                
                int replacingID = getObjectByIndex( &mCurrentTransition,
                                                    mCurrentlyReplacing );
                
                if( replacingID > 0 ) {    
                    redoTransSearches( replacingID, true );
                    }
                
                checkIfSaveVisible();
                return;
                }
            }
        
        for( int i=0; i<4; i++ ) {
            if( inTarget == mClearButtons[i] ) {
                
                setObjectByIndex( &mCurrentTransition, i, 0 );
            
                checkIfSaveVisible();
                return;
                }
            }

        for( int i=0; i< NUM_TREE_TRANS_TO_SHOW; i++ ) {
            if( inTarget == mProducedByButtons[i] ) {
                
                mCurrentTransition = mProducedBy[i];
                
                // -1 transition means blank target (eating)
                if( mCurrentTransition.target == -1 ) {
                    mCurrentTransition.target = 0;
                    }
                if( mCurrentTransition.actor < 0 ) {
                    // auto-decay and default transition have blank actor
                    mCurrentTransition.actor = 0;
                    }

                // select the obj we were searching for when
                // we jump to this transition... thus, we don't redo search
                for( int j=0; j<4; j++ ) {
                    if( getObjectByIndex( &mCurrentTransition, j ) == 
                        mLastSearchID ) {
                        
                        mCurrentlyReplacing = j;
                        break;
                        }
                    }
                checkIfSaveVisible();
                return;
                }
            if( inTarget == mProducesButtons[i] ) {
            
                mCurrentTransition = mProduces[i];
                
                // -1 transition means blank target (eating)
                if( mCurrentTransition.target == -1 ) {
                    mCurrentTransition.target = 0;
                    }
                if( mCurrentTransition.actor < 0 ) {
                    // auto-decay and default transition have blank actor
                    mCurrentTransition.actor = 0;
                    }

                // select the obj we were searching for when
                // we jump to this transition... thus, we don't redo search
                for( int j=0; j<4; j++ ) {
                    if( getObjectByIndex( &mCurrentTransition, j ) == 
                        mLastSearchID ) {
                        
                        mCurrentlyReplacing = j;
                        break;
                        }
                    }
                checkIfSaveVisible();
                return;
                }
            }
        }
    
    }



static void drawTransObject( int inID, doublePair inPos ) {
    if( inID > 0 ) {

        ObjectRecord *r = getObject( inID );
        
        int maxD = getMaxDiameter( r );
        
        double zoom = 1;
        
        if( maxD > 128 ) {
            zoom = 128.0 / maxD;
            }
        
        inPos = sub( inPos, mult( getObjectCenterOffset( r ), zoom ) );

        drawObject( getObject( inID ), 2, inPos, 0, false, false, 20, 0, false,
                    false,
                    getEmptyClothingSet(), zoom );
        }
    }



   

     
void EditorTransitionPage::draw( doublePair inViewCenter, 
                     double inViewSize ) {
    
    for( int i=0; i<4; i++ ) {
        doublePair pos = mPickButtons[i]->getCenter();        

        if( i == mCurrentlyReplacing ) {
            setDrawColor( 1, 1, 0, 1 );
            drawSquare( pos, 60 );
            }
        
        
        setDrawColor( 1, 1, 1, 1 );
        drawSquare( pos, 50 );
        
        int id = getObjectByIndex( &mCurrentTransition, i );
        
        drawTransObject( id, pos );

        if( id > 0 ) {
            if( i > 1 ) {
                pos.y -= 70;
                
                if( i == 2 ) {
                    pos.y -= 25;
                    }
                }
            else {
                pos.y += 70;
                if( i == 0 ) {
                    pos.y += 25;
                    }
                }

            ObjectRecord *r = getObject( id );
            
            setDrawColor( 1.0, 1.0, 1.0, 1.0 );
            smallFont->drawString( r->description, pos, alignCenter );
            }
        
        }

    doublePair centerA = mult( add( mPickButtons[0]->getCenter(),
                                    mPickButtons[1]->getCenter() ),
                               0.5 );
    
    setDrawColor( 1, 1, 1, 1 );
    mainFont->drawString( "+", centerA, alignCenter );


    doublePair centerB = mult( add( mPickButtons[2]->getCenter(),
                                    mPickButtons[3]->getCenter() ),
                               0.5 );
    
    mainFont->drawString( "+", centerB, alignCenter );
    
    
    doublePair centerC = mult( add( centerA, centerB ), 0.5 );
    
    mainFont->drawString( "=", centerC, alignCenter );


    
    
    for( int i=0; i<NUM_TREE_TRANS_TO_SHOW; i++ ) {

        if( mProducedByButtons[i]->isVisible() ) {
        
            int actor = getObjectByIndex( &mProducedBy[i], 0 );
            int target = getObjectByIndex( &mProducedBy[i], 1 );
            
            doublePair pos = mProducedByButtons[i]->getCenter();
            
            pos.x -= 75;
            setDrawColor( 1, 1, 1, 1 );
            drawSquare( pos, 50 );

            
            drawTransObject( actor, pos );
            
            pos.x += 150;
            setDrawColor( 1, 1, 1, 1 );
            drawSquare( pos, 50 );

            // target always non-blank
            drawTransObject( target, pos );
            }

        
        if( mProducesButtons[i]->isVisible() ) {
            
            int newActor = getObjectByIndex( &mProduces[i], 2 );
            int newTarget = getObjectByIndex( &mProduces[i], 3 );
        
            doublePair pos = mProducesButtons[i]->getCenter();
        
            pos.x -= 75;
            
            setDrawColor( 1, 1, 1, 1 );
            drawSquare( pos, 50 );
            
            drawTransObject( newActor, pos );
            
            pos.x += 150;
            
            setDrawColor( 1, 1, 1, 1 );
            drawSquare( pos, 50 );
            
            drawTransObject( newTarget, pos );
            }
        }
    
    

    if( mCurrentTransition.target != 0 &&
        getObject( mCurrentTransition.target )->person ) {

        doublePair pos = mPickButtons[1]->getCenter();

        pos.y += 95;
        
        setDrawColor( 1, 1, 1, 1 );
        
        smallFont->drawString( "Generic on-person Transition", 
                               pos, alignCenter );
        }
    else if( mCurrentTransition.target != 0 &&
             mCurrentTransition.newActor == 0 &&
             mCurrentTransition.actor > 0 &&
             getObject( mCurrentTransition.actor )->person ) {

        doublePair pos = mPickButtons[1]->getCenter();

        pos.y += 95;
        
        setDrawColor( 1, 1, 1, 1 );
        
        smallFont->drawString( "Default Transition", 
                               pos, alignCenter );
        }
    else if( mCurrentTransition.target == 0 &&
             mCurrentTransition.actor > 0 &&
             getObject( mCurrentTransition.actor )->foodValue > 0 &&
             mCurrentTransition.newActor != 0 &&
             mCurrentTransition.newTarget == 0 ) {
        
        doublePair pos = mPickButtons[0]->getCenter();

        pos.y += 120;
        
        setDrawColor( 1, 1, 1, 1 );
        
        smallFont->drawString( "Eating Transition", 
                               pos, alignCenter );
        
        }
    else if( mCurrentTransition.target == 0 &&
             mCurrentTransition.actor > 0 &&
             getObject( mCurrentTransition.actor )->foodValue == 0 &&
             mCurrentTransition.newActor != 0 &&
             mCurrentTransition.newTarget == 0 ) {
        
        doublePair pos = mPickButtons[0]->getCenter();

        pos.y += 120;
        
        setDrawColor( 1, 1, 1, 1 );
        
        smallFont->drawString( "Generic Use Transition", 
                               pos, alignCenter );
        
        }
    else if( mCurrentTransition.target == 0 &&
             mCurrentTransition.actor > 0 &&
             getObject( mCurrentTransition.actor )->foodValue == 0 &&
             mCurrentTransition.newTarget != 0 ) {
        
        doublePair pos = mPickButtons[0]->getCenter();

        pos.y += 120;
        
        setDrawColor( 1, 1, 1, 1 );
        
        smallFont->drawString( "Use on Bare Ground", 
                               pos, alignCenter );
        
        }
    
    setDrawColor( 1, 1, 1, 1 );
    
    if( mLastUseActorCheckbox.isVisible() ) {
        doublePair pos = mLastUseActorCheckbox.getPosition();
        pos.x -= checkboxSep;
        smallFont->drawString( "Last Use", pos, alignRight );
        }
    if( mLastUseTargetCheckbox.isVisible() ) {
        doublePair pos = mLastUseTargetCheckbox.getPosition();
        pos.x += checkboxSep;
        smallFont->drawString( "Last Use", pos, alignLeft );
        }

    if( mReverseUseActorCheckbox.isVisible() ) {
        doublePair pos = mReverseUseActorCheckbox.getPosition();
        pos.x -= checkboxSep;
        smallFont->drawString( "Reverse Use", pos, alignRight );
        }

    if( mReverseUseTargetCheckbox.isVisible() ) {
        doublePair pos = mReverseUseTargetCheckbox.getPosition();
        pos.x += checkboxSep;
        smallFont->drawString( "Reverse Use", pos, alignLeft );
        }
    

    if( mAutoDecayTimeField.isVisible() && 
        mAutoDecayTimeField.getInt() < 0 ) {
        
        int numEpocs = - mAutoDecayTimeField.getInt();
        
        doublePair pos = mAutoDecayTimeField.getPosition();
        pos.x += 38;

        const char *plural = "";
        
        if( numEpocs != 1 ) {
            plural = "s";
            }
        char *displayString = autoSprintf( "(%d epoc%s)",
                                           numEpocs,
                                           plural );

        smallFont->drawString( displayString, pos, alignLeft );
        delete [] displayString;
        }
    
    }



void EditorTransitionPage::step() {
    }




void EditorTransitionPage::makeActive( char inFresh ) {
    if( !inFresh ) {
        return;
        }
    
    mObjectPicker.redoSearch( false );

    }



void EditorTransitionPage::pointerMove( float inX, float inY ) {
    }


void EditorTransitionPage::pointerDown( float inX, float inY ) {

    }




void EditorTransitionPage::pointerDrag( float inX, float inY ) {
    //doublePair pos = {inX, inY};
    }



void EditorTransitionPage::pointerUp( float inX, float inY ) {
    }



void EditorTransitionPage::keyDown( unsigned char inASCII ) {
        
    
        
    }



void EditorTransitionPage::specialKeyDown( int inKeyCode ) {
    
            
    }
