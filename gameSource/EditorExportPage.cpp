#include "EditorExportPage.h"

#include "minorGems/game/game.h"
#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"

#include "minorGems/util/stringUtils.h"



#include "message.h"

#include "exporter.h"







extern Font *mainFont;
extern Font *smallFont;
extern Font *smallFontFixed;



#include "ObjectPickable.h"

static ObjectPickable objectPickable;




static char unpickable( int inID ) {
    if( doesExportContain( inID ) ) {
        return true;
        }
    return false;
    }



EditorExportPage::EditorExportPage()
        : mObjectPicker( &objectPickable, +410, 90 ),
          mObjectEditorButton( mainFont, 0, 260, "Objects" ),
          mSearchNeedsRedo( false ) {
    
    mObjectPicker.addFilter( &unpickable );
    

    addComponent( &mObjectPicker );
    addComponent( &mObjectEditorButton );
    

    mObjectPicker.addActionListener( this );
    
    mObjectEditorButton.addActionListener( this );
    
    mSelectionIndex = 0;

    addKeyClassDescription( &mKeyLegend, "Up/Down", "Change selection" );
    addKeyClassDescription( &mKeyLegend, "Ctr/Shft", "Bigger jumps" );
    addKeyClassDescription( &mKeyLegend, "Bkspce", "Remv item" );
    }



EditorExportPage::~EditorExportPage() {
    }



void EditorExportPage::actionPerformed( GUIComponent *inTarget ) {
    
    if( inTarget == &mObjectPicker ) {
        int obj = mObjectPicker.getSelectedObject();
            
        if( obj != -1 ) {
            addExportObject( obj );

            mSearchNeedsRedo = true;
            }
        updateVisible();
        }
    else if( inTarget == &mObjectEditorButton ) {
        setSignal( "objectEditor" );
        }    
    }


void EditorExportPage::updateVisible() {
    if( getCurrentExportList()->size() > 0 ) {
        
        }
    }



static void drawObjectList( SimpleVector<int> *inList,
                            int inSelectionIndex = -1 ) {

    int num = inList->size();
        
    FloatColor boxColor = { 0.25f, 0.25f, 0.25f, 1.0f };
    
    FloatColor altBoxColor = { 0.35f, 0.35f, 0.35f, 1.0f };
    
    FloatColor selBoxColor = { 0.35f, 0.35f, 0.0f, 1.0f };

    doublePair pos;
    
    pos.x = -100;
    pos.y = 150;
        
    double spacing = 30;

    if( inSelectionIndex > 7 && num > 12 ) {    
        pos.y += ( inSelectionIndex - 7 )  * spacing;
        }
        

    for( int i=0; i<num; i++ ) {
            
        int objID = inList->getElementDirect( i );

        if( inSelectionIndex == i ) {
            setDrawColor( selBoxColor );
            }
        else {

            setDrawColor( boxColor );
            }

        float fade = 1;
            
        if( num > 12 && pos.y < -130 ) {
            fade = 1 - ( -130 - pos.y ) / 95;
                
            if( fade < 0 ) {
                fade = 0;
                }
            }

        if( pos.y > 130 ) {
            fade = 1 - ( pos.y - 130 ) / 95;
                
            if( fade < 0 ) {
                fade = 0;
                }
            }
            
            
        setDrawFade( fade );
        drawRect( pos, 150, spacing / 2  );


            
        FloatColor tempColor = boxColor;
        boxColor = altBoxColor;
            
        altBoxColor = tempColor;
            

        if( inSelectionIndex == i ) {
            setDrawColor( 1, 1, 0, 1 );
            }
        else {
            setDrawColor( 1, 1, 1, 1 );
            }
            
        doublePair textPos = pos;
        textPos.x -= 140;
            
        setDrawFade( fade );

        smallFont->drawString( getObject( objID )->description, 
                               textPos, alignLeft );
        

        doublePair idPos = textPos;
        
        idPos.x -= 17;
        
        char *idString = autoSprintf( "%d", objID );
        
        smallFontFixed->drawString( idString, idPos, alignRight );
        
        delete [] idString;
        
        
        pos.y -= spacing;
        }
    }


   

     
void EditorExportPage::draw( doublePair inViewCenter, 
                             double inViewSize ) {

    if( getCurrentExportList()->size() > 0 ) {        
        doublePair legendPos = mObjectEditorButton.getPosition();
        
        legendPos.x = -350;
        legendPos.y += 50;
        
        drawKeyLegend( &mKeyLegend, legendPos );

        drawObjectList( getCurrentExportList(), mSelectionIndex );
        }
    }




void EditorExportPage::step() {
    if( mSearchNeedsRedo ) {
        mObjectPicker.redoSearch( false );
        
        mSearchNeedsRedo = false;
        }
    }




void EditorExportPage::makeActive( char inFresh ) {
    if( !inFresh ) {
        return;
        }
    
    mObjectPicker.redoSearch( false );
    }



void EditorExportPage::pointerMove( float inX, float inY ) {
    }


void EditorExportPage::pointerDown( float inX, float inY ) {

    }




void EditorExportPage::pointerDrag( float inX, float inY ) {
    //doublePair pos = {inX, inY};
    }



void EditorExportPage::pointerUp( float inX, float inY ) {
    }



void EditorExportPage::keyDown( unsigned char inASCII ) {
    if( TextField::isAnyFocused() ) {
        return;
        }
    
    if( inASCII == 8 ) {
        // backspace
        
        SimpleVector<int> *currentList = getCurrentExportList();
        
        if( currentList->size() > 0 ) {
            // delete object from export
            
            char lastElement = false;
            
            if( mSelectionIndex == currentList->size() - 1 ) {
                lastElement = true;
                }

            int id = currentList->getElementDirect( mSelectionIndex );
            
            removeExportObject( id );
            
            if( mSelectionIndex > 0 && lastElement ) {
                mSelectionIndex--;
                }
            
            mObjectPicker.redoSearch( false );
            }
        }    
    }



void EditorExportPage::specialKeyDown( int inKeyCode ) {


    int offset = 1;
    
    if( isCommandKeyDown() ) {
        offset = 5;
        }
    if( isShiftKeyDown() ) {
        offset = 10;
        }
    if( isCommandKeyDown() && isShiftKeyDown() ) {
        offset = 20;
        }
    

    switch( inKeyCode ) {
        case MG_KEY_DOWN: {
            
            mSelectionIndex += offset;
            
            int max = getCurrentExportList()->size() - 1;
            
            if( mSelectionIndex > max ) {
                mSelectionIndex = max;
                
                if( mSelectionIndex > max  || mSelectionIndex < 0 ) {
                    mSelectionIndex = 0;
                    }
                }

            break;
            }
        case MG_KEY_UP: {
            mSelectionIndex -= offset;
            
            if( mSelectionIndex < 0 ) {
                mSelectionIndex = 0;
                }
            break;
            }
        }
            
    }
