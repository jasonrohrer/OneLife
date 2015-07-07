#include "EditorTransitionPage.h"

#include "minorGems/game/game.h"
#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"

#include "minorGems/graphics/converters/PNGImageConverter.h"

#include "minorGems/io/file/FileInputStream.h"



extern Font *mainFont;
extern Font *smallFont;



#include "ObjectPickable.h"

static ObjectPickable objectPickable;



EditorTransitionPage::EditorTransitionPage()
        : mSaveTransitionButton( smallFont, 210, -260, "Save New" ),
          mObjectPicker( &objectPickable, +310, 100 ),
          mObjectEditorButton( mainFont, 0, 260, "Objects" ) {

    addComponent( &mSaveTransitionButton );
    addComponent( &mObjectPicker );
    addComponent( &mObjectEditorButton );
    

    mSaveTransitionButton.addActionListener( this );
    mObjectEditorButton.addActionListener( this );
    
    mObjectPicker.addActionListener( this );

    mSaveTransitionButton.setVisible( false );


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
    }



EditorTransitionPage::~EditorTransitionPage() {
    for( int i=0; i<4; i++ ) {
        delete mReplaceButtons[i];
        }
    for( int i=0; i<2; i++ ) {
        delete mClearButtons[i];
        }
    }






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





void EditorTransitionPage::actionPerformed( GUIComponent *inTarget ) {
    
    if( inTarget == &mSaveTransitionButton ) {
        
        }
    else if( inTarget == &mObjectPicker ) {
        if( mCurrentlyReplacing != -1 ) {
            
            int objectID = mObjectPicker.getSelectedObject();
            
            if( objectID != -1 ) {
                setObjectByIndex( &mCurrentTransition, mCurrentlyReplacing,
                                  objectID );
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
                
                return;
                }
            }
        
        for( int i=0; i<2; i++ ) {
            if( inTarget == mClearButtons[i] ) {
                
                setObjectByIndex( &mCurrentTransition, i+2, -1 );
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
    doublePair pos = {inX, inY};
    }



void EditorTransitionPage::pointerUp( float inX, float inY ) {
    }



void EditorTransitionPage::keyDown( unsigned char inASCII ) {
        
    
        
    }



void EditorTransitionPage::specialKeyDown( int inKeyCode ) {
    
            
    }
