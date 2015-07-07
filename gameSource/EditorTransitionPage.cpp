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
        : mSaveTransitionButton( mainFont, -210, 0, "Save" ),
          mObjectPicker( &objectPickable, +310, 100 ),
          mObjectEditorButton( mainFont, 310, 260, "Objects" ),
          mProducedByNext( smallFont, 200, 260, "Next" ),
          mProducedByPrev( smallFont, -350, 260, "Prev" ),
          mProducesNext( smallFont, 200, -260, "Next" ),
          mProducesPrev( smallFont, -350, -260, "Prev" ) {

    addComponent( &mSaveTransitionButton );
    addComponent( &mObjectPicker );
    addComponent( &mObjectEditorButton );


    addComponent( &mProducedByNext );
    addComponent( &mProducedByPrev );
    addComponent( &mProducesNext );
    addComponent( &mProducesPrev );
    

    mSaveTransitionButton.addActionListener( this );
    mObjectEditorButton.addActionListener( this );
    
    mObjectPicker.addActionListener( this );

    mSaveTransitionButton.setVisible( false );


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
    for( int y = 0; y<2; y++ ) {
        int xP = - 250;
        for( int x = 0; x<2; x++ ) {
        
            mReplaceButtons[i] = new TextButton( mainFont, xP, yP, "R" );
            
            addComponent( mReplaceButtons[i] );
            
            mReplaceButtons[i]->addActionListener( this );
            
            if( i>= 2 ) {
                mClearButtons[i-2] = 
                    new TextButton( smallFont, xP, yP - 52, "X" );
                
                addComponent( mClearButtons[i-2] );
                mClearButtons[i-2]->addActionListener( this );
                }
            
            
            i++;
            
            xP += 400;
            }
        yP -= 150;
        }


    int xP = -200;
    for( int i=0; i<NUM_TREE_TRANS_TO_SHOW; i++ ) {
        mProducedByButtons[i] = new TextButton( smallFont, xP, 260, "L" );
        mProducesButtons[i] = new TextButton( smallFont, xP, -260, "L" );
        
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
    
    }



EditorTransitionPage::~EditorTransitionPage() {
    for( int i=0; i<4; i++ ) {
        delete mReplaceButtons[i];
        }
    for( int i=0; i<2; i++ ) {
        delete mClearButtons[i];
        }
    
    for( int i=0; i<NUM_TREE_TRANS_TO_SHOW; i++ ) {
        delete mProducedByButtons[i];
        delete mProducesButtons[i];
        }
    }









void EditorTransitionPage::checkIfSaveVisible() {
    
    if( getObjectByIndex( &mCurrentTransition, 0 ) != -1
        &&
        getObjectByIndex( &mCurrentTransition, 1 ) != -1 ) {
        
        mSaveTransitionButton.setVisible( true );
        }
    else {
        mSaveTransitionButton.setVisible( false );
        }
    }



void EditorTransitionPage::redoTransSearches( int inObjectID,
                                              char inClearSkip ) {

    printf( "Searching for %d\n", inObjectID );
    
    if( inClearSkip ) {
        mProducedBySkip = 0;
        mProducesSkip = 0;
        }


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
    
        mProducedByPrev.setVisible( numLeft > 0 );

        delete [] resultsA;
        }
    else {
        mProducedByPrev.setVisible( false );
    
        mProducedByPrev.setVisible( false );
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
        
        mProducesPrev.setVisible( numLeft > 0 );
        
        delete [] resultsA;
                }
    else {
        mProducesPrev.setVisible( false );
    
        mProducesPrev.setVisible( false );
        }

    
    }





void EditorTransitionPage::actionPerformed( GUIComponent *inTarget ) {
    
    if( inTarget == &mSaveTransitionButton ) {
        

        addTrans( mCurrentTransition.actor,
                  mCurrentTransition.target,
                  mCurrentTransition.newActor,
                  mCurrentTransition.newTarget );
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
    else {

        for( int i=0; i<4; i++ ) {
            if( inTarget == mReplaceButtons[i] ) {
                
                mCurrentlyReplacing = i;
                
                mObjectPicker.unselectObject();

                
                int replacingID = getObjectByIndex( &mCurrentTransition,
                                                    mCurrentlyReplacing );
                
                if( replacingID != -1 ) {    
                    redoTransSearches( replacingID, true );
                    }
                
                return;
                }
            }
        
        for( int i=0; i<2; i++ ) {
            if( inTarget == mClearButtons[i] ) {
                
                setObjectByIndex( &mCurrentTransition, i+2, -1 );
                checkIfSaveVisible();
                return;
                }
            }
        
        }
    
    }



   

     
void EditorTransitionPage::draw( doublePair inViewCenter, 
                     double inViewSize ) {
    
    for( int i=0; i<4; i++ ) {
        doublePair pos = mReplaceButtons[i]->getCenter();        
        
        if( i % 2 == 0 ) {    
            pos.x += 100;
            }
        else {
            pos.x -= 100;
            }
        
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

    doublePair centerA = mult( add( mReplaceButtons[0]->getCenter(),
                                    mReplaceButtons[1]->getCenter() ),
                               0.5 );
    
    setDrawColor( 1, 1, 1, 1 );
    mainFont->drawString( "+", centerA, alignCenter );


    doublePair centerB = mult( add( mReplaceButtons[2]->getCenter(),
                                    mReplaceButtons[3]->getCenter() ),
                               0.5 );
    
    mainFont->drawString( "+", centerB, alignCenter );
    
    
    doublePair centerC = mult( add( centerA, centerB ), 0.5 );
    
    mainFont->drawString( "=", centerC, alignCenter );


    setDrawColor( 1, 1, 1, 1 );
    
    for( int i=0; i<NUM_TREE_TRANS_TO_SHOW; i++ ) {
        int actor = getObjectByIndex( &mProducedBy[i], 0 );
        int target = getObjectByIndex( &mProducedBy[i], 1 );
        
        if( actor != 1 && target != -1 ) {
            
            doublePair pos = mProducedByButtons[i]->getCenter();
            
            pos.x += 100;
            
            drawSquare( pos, 50 );

            drawObject( getObject( actor ), pos );
            
            pos.x += 150;
            
            drawSquare( pos, 50 );

            drawObject( getObject( target ), pos );
            }


        int newActor = getObjectByIndex( &mProduces[i], 2 );
        int newTarget = getObjectByIndex( &mProduces[i], 3 );
        
        if( newActor != -1 || newTarget != -1 ) {
            
            doublePair pos = mProducesButtons[i]->getCenter();
            
            pos.x += 100;
            
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
