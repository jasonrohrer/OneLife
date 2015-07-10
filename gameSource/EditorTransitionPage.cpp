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
        : mSaveTransitionButton( mainFont, -310, 0, "Save" ),
          mObjectPicker( &objectPickable, +310, 100 ),
          mObjectEditorButton( mainFont, 310, 260, "Objects" ),
          mProducedByNext( smallFont, 200, 260, "Next" ),
          mProducedByPrev( smallFont, -350, 260, "Prev" ),
          mProducesNext( smallFont, 200, -260, "Next" ),
          mProducesPrev( smallFont, -350, -260, "Prev" ),
          mDelButton( smallFont, +150, 0, "Delete" ),
          mDelConfirmButton( smallFont, +150, 40, "Delete?" ) {

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
    

    mCurrentTransition.actor = -1;
    mCurrentTransition.target = -1;
    mCurrentTransition.newActor = -1;
    mCurrentTransition.newTarget = -1;
    
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
            setObjectByIndex( &( mProducedBy[i] ), j, -1 );
            setObjectByIndex( &( mProduces[i] ), j, -1 );
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
            
            setObjectByIndex( &mCurrentTransition, i, -1 );
            }
        }
    checkIfSaveVisible();
    }




void EditorTransitionPage::checkIfSaveVisible() {
    char vis = ( getObjectByIndex( &mCurrentTransition, 1 ) != -1 );
    
    mSaveTransitionButton.setVisible( vis );
    
    mDelButton.setVisible( vis );
    mDelConfirmButton.setVisible( false );
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
            setObjectByIndex( &( mProducedBy[i] ), j, -1 );
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
            setObjectByIndex( &( mProduces[i] ), j, -1 );
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
        

        addTrans( mCurrentTransition.actor,
                  mCurrentTransition.target,
                  mCurrentTransition.newActor,
                  mCurrentTransition.newTarget );
            
        redoTransSearches( mLastSearchID, true );
        }
    if( inTarget == &mDelButton ) {
        mDelButton.setVisible( false );
        mDelConfirmButton.setVisible( true );
        }
    else if( inTarget == &mDelConfirmButton ) {
        
        deleteTransFromBank( getObjectByIndex( &mCurrentTransition, 0 ),
                             getObjectByIndex( &mCurrentTransition, 1 ) );
                             
        for( int i=0; i<4; i++ ) {
            setObjectByIndex( &mCurrentTransition, i, -1 );
            }
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
                
                if( replacingID != -1 ) {    
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
                
                setObjectByIndex( &mCurrentTransition, index, -1 );
            
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
        
        if( id != -1 ) {
            drawObject( getObject( id ), pos );
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

            
            if( actor != -1 ) {
                drawObject( getObject( actor ), pos );
                }
            
            pos.x += 150;
            
            drawSquare( pos, 50 );

            // target always non-blank
            drawObject( getObject( target ), pos );
            }

        
        if( mProducesButtons[i]->isVisible() ) {
            
            int newActor = getObjectByIndex( &mProduces[i], 2 );
            int newTarget = getObjectByIndex( &mProduces[i], 3 );
        
            doublePair pos = mProducesButtons[i]->getCenter();
        
            pos.x -= 75;
            
            drawSquare( pos, 50 );
            
            if( newActor != -1 ) {
                drawObject( getObject( newActor ), pos );
                }
            
            pos.x += 150;
            
            drawSquare( pos, 50 );
            
            if( newTarget != -1 ) {
                drawObject( getObject( newTarget ), pos );
                }
            }
        }
    
    if( mCurrentlyReplacing != -1 ) {
        int id = getObjectByIndex( &mCurrentTransition, mCurrentlyReplacing );
        
        if( id != -1 ) {
            
            ObjectRecord *r = getObject( id );
            
            setDrawColor( 1, 1, 1, 1 );
            
            doublePair desPos = centerB;
            
            desPos.y -= 100;

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
