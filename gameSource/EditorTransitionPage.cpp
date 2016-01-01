#include "EditorTransitionPage.h"

#include "minorGems/game/game.h"
#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"

#include "minorGems/graphics/converters/PNGImageConverter.h"

#include "minorGems/io/file/FileInputStream.h"





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



EditorTransitionPage::EditorTransitionPage()
        : mAutoDecayTimeField( smallFont, 
                               0,  -160, 6,
                               false,
                               "AutoDecay Seconds", "0123456789", NULL ),
          mSaveTransitionButton( mainFont, -310, 0, "Save" ),
          mObjectPicker( &objectPickable, +310, 100 ),
          mObjectEditorButton( mainFont, -300, 260, "Objects" ),
          mProducedByNext( smallFont, 180, 260, "Next" ),
          mProducedByPrev( smallFont, -180, 260, "Prev" ),
          mProducesNext( smallFont, 180, -260, "Next" ),
          mProducesPrev( smallFont, -180, -260, "Prev" ),
          mDelButton( smallFont, +150, 0, "Delete" ),
          mDelConfirmButton( smallFont, +150, 40, "Delete?" ) {


    addComponent( &mAutoDecayTimeField );
    mAutoDecayTimeField.setVisible( false );
    
    addComponent( &mSaveTransitionButton );
    addComponent( &mObjectPicker );
    addComponent( &mObjectEditorButton );
    addComponent( &mDelButton );
    addComponent( &mDelConfirmButton );

    addComponent( &mProducedByNext );
    addComponent( &mProducedByPrev );
    addComponent( &mProducesNext );
    addComponent( &mProducesPrev );
    

    mSaveTransitionButton.addActionListener( this );
    mObjectEditorButton.addActionListener( this );
    
    mObjectPicker.addActionListener( this );

    mSaveTransitionButton.setVisible( false );
    
    mDelButton.addActionListener( this );
    mDelConfirmButton.addActionListener( this );

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

    mCurrentlyReplacing = 0;
    

    
    int yP = 75;
    
    int i = 0;
    int clearI = 0;
    
    for( int y = 0; y<2; y++ ) {
        int xP = - 200;
        for( int x = 0; x<2; x++ ) {
        
            // buttons hiding under displays to register clicks
            mPickButtons[i] = new Button( xP, yP, 100, 100, 1 );
            
            addComponent( mPickButtons[i] );
            
            mPickButtons[i]->addActionListener( this );
            
            if( i == 0 || i>= 2 ) {
                int offset = -90;
                if( i == 3 ) {
                    offset = 90;
                    }
                
                mClearButtons[clearI] = 
                    new TextButton( smallFont, xP + offset, yP, "X" );
                
                addComponent( mClearButtons[clearI] );
                mClearButtons[clearI]->addActionListener( this );

                clearI++;
                }
            
            
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
    for( int i=0; i<3; i++ ) {
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
    char vis = ( getObjectByIndex( &mCurrentTransition, 1 ) != 0 );
    
    mSaveTransitionButton.setVisible( vis );
    
    mDelButton.setVisible( vis );
    mDelConfirmButton.setVisible( false );

    if( vis && 
        getObjectByIndex( &mCurrentTransition, 0 ) <= 0 &&
        getObjectByIndex( &mCurrentTransition, 2 ) == 0 ) {
        // no actor, no new actor, but there's a target

        // can be auto-decay
        mAutoDecayTimeField.setVisible( true );

        if( mCurrentTransition.autoDecaySeconds > 0 ) {
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
        
        if( decayTime > 0 ) {
            // signal auto-delay
            actor = -1;
            }
        else if( actor == -1 ) {
            // correct it back to 0 for non-autoDecay transitions
            actor = 0;
            }
        
        addTrans( actor,
                  mCurrentTransition.target,
                  mCurrentTransition.newActor,
                  mCurrentTransition.newTarget,
                  decayTime );
            
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
        
        if( decayTime > 0 ) {
            // signal auto-delay
            actor = -1;
            }
        else if( actor == -1 ) {
            // correct it back to 0 for non-autoDecay transitions
            actor = 0;
            }

        deleteTransFromBank( actor,
                             getObjectByIndex( &mCurrentTransition, 1 ) );
                             
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
    else if( inTarget == &mObjectEditorButton ) {
        setSignal( "objectEditor" );
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
                
                if( replacingID != 0 ) {    
                    redoTransSearches( replacingID, true );
                    }
                
                checkIfSaveVisible();
                return;
                }
            }
        
        for( int i=0; i<3; i++ ) {
            if( inTarget == mClearButtons[i] ) {
                
                int index = i;
                
                if( i !=  0 ) {
                    // skip target, can't be cleared
                    index = i+1;
                    }
                
                setObjectByIndex( &mCurrentTransition, index, 0 );
            
                checkIfSaveVisible();
                return;
                }
            }

        for( int i=0; i< NUM_TREE_TRANS_TO_SHOW; i++ ) {
            if( inTarget == mProducedByButtons[i] ) {
                
                mCurrentTransition = mProducedBy[i];
                
                // select the obj we were searching for when
                // we jump too this transition... thus, we don't redo search
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
                
                // select the obj we were searching for when
                // we jump too this transition... thus, we don't redo search
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
        
        if( id > 0 ) {
            drawObject( getObject( id ), pos, false, -1, 
                        getEmptyClothingSet() );
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


    setDrawColor( 1, 1, 1, 1 );
    
    for( int i=0; i<NUM_TREE_TRANS_TO_SHOW; i++ ) {

        if( mProducedByButtons[i]->isVisible() ) {
        
            int actor = getObjectByIndex( &mProducedBy[i], 0 );
            int target = getObjectByIndex( &mProducedBy[i], 1 );
            
            doublePair pos = mProducedByButtons[i]->getCenter();
            
            pos.x -= 75;
            
            drawSquare( pos, 50 );

            
            if( actor > 0 ) {
                drawObject( getObject( actor ), pos, false, -1,
                            getEmptyClothingSet() );
                }
            
            pos.x += 150;
            
            drawSquare( pos, 50 );

            // target always non-blank
            drawObject( getObject( target ), pos, false, -1,
                        getEmptyClothingSet() );
            }

        
        if( mProducesButtons[i]->isVisible() ) {
            
            int newActor = getObjectByIndex( &mProduces[i], 2 );
            int newTarget = getObjectByIndex( &mProduces[i], 3 );
        
            doublePair pos = mProducesButtons[i]->getCenter();
        
            pos.x -= 75;
            
            drawSquare( pos, 50 );
            
            if( newActor != 0 ) {
                drawObject( getObject( newActor ), pos, false, -1,
                            getEmptyClothingSet() );
                }
            
            pos.x += 150;
            
            drawSquare( pos, 50 );
            
            if( newTarget != 0 ) {
                drawObject( getObject( newTarget ), pos, false, -1,
                            getEmptyClothingSet() );
                }
            }
        }
    
    if( mCurrentlyReplacing != -1 ) {
        int id = getObjectByIndex( &mCurrentTransition, mCurrentlyReplacing );
        
        if( id != 0 ) {
            
            ObjectRecord *r = getObject( id );
            
            setDrawColor( 1, 1, 1, 1 );
            
            doublePair desPos = centerA;
            
            desPos.y += 100;

            smallFont->drawString( r->description, desPos, alignCenter );

            
            }
        }

    }



void EditorTransitionPage::step() {
    }




void EditorTransitionPage::makeActive( char inFresh ) {
    if( !inFresh ) {
        return;
        }
    
    mObjectPicker.redoSearch();

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
