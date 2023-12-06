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
          mExportButton( mainFont, 210, -260, "Export" ),
          mClearButton( mainFont, -450, -260, "Clear" ),
          mCopyHashButton( smallFont, 400, -290, "Copy Hash" ),
          mCopyIDListButton( smallFont, -300, -230, "Copy ID List" ),
          mPasteIDListButton( smallFont, -300, -290, "Paste ID List" ),
          mExportTagField( mainFont, 
                           0,  -260, 6,
                           false,
                           "Tag", 
                           "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                           "abcdefghijklmnopqrstuvwxyz"
                           "0123456789_", 
                           NULL ),
          mSearchNeedsRedo( false ),
          mCurrentHash( NULL ),
          mCurrentErrorMessage( NULL ){
    
    mObjectPicker.addFilter( &unpickable );
    

    addComponent( &mObjectPicker );
    addComponent( &mObjectEditorButton );
    
    addComponent( &mExportButton );
    addComponent( &mClearButton );
    addComponent( &mCopyHashButton );
    addComponent( &mCopyIDListButton );
    addComponent( &mPasteIDListButton );
    
    addComponent( &mExportTagField );
    
    mExportTagField.setFireOnAnyTextChange( true );
    mExportTagField.usePasteShortcut( true );


    mObjectPicker.addActionListener( this );
    
    mObjectEditorButton.addActionListener( this );
    
    mExportButton.addActionListener( this );
    mClearButton.addActionListener( this );
    mCopyHashButton.addActionListener( this );
    mCopyIDListButton.addActionListener( this );
    mPasteIDListButton.addActionListener( this );

    mExportTagField.addActionListener( this );
    

    mSelectionIndex = 0;

    addKeyClassDescription( &mKeyLegend, "Up/Down", "Change selection" );
    addKeyClassDescription( &mKeyLegend, "Ctr/Shft", "Bigger jumps" );
    addKeyClassDescription( &mKeyLegend, "Bkspce", "Remv item" );
    
    
    updateVisible();
    }



EditorExportPage::~EditorExportPage() {
    if( mCurrentHash != NULL ) {
        delete [] mCurrentHash;
        }
    
    clearErrorMessage();
    }



void EditorExportPage::actionPerformed( GUIComponent *inTarget ) {
    
    if( inTarget == &mObjectPicker ) {
        int obj = mObjectPicker.getSelectedObject();
            
        if( obj != -1 ) {
            int oldListLen = getCurrentExportList()->size();
            
            addExportObject( obj );

            int newListLen = getCurrentExportList()->size();
            
            if( newListLen > oldListLen ) {
                // new object added!
                // jump to it
                mSelectionIndex = newListLen - 1;
                }

            mSearchNeedsRedo = true;
            clearErrorMessage();
            }
        updateVisible();
        }
    else if( inTarget == &mObjectEditorButton ) {
        clearErrorMessage();
        setSignal( "objectEditor" );
        }
    else if( inTarget == &mExportButton ) {
        char *tagText = mExportTagField.getText();
        
        char succuess = finalizeExportBundle( tagText );
        
        delete [] tagText;
        
        clearErrorMessage();
        
        if( ! succuess ) {
            setErrorMessage( "Export failed." );
            }

        mObjectPicker.redoSearch( false );
        
        updateVisible();
        }
    else if( inTarget == &mClearButton ) {
        clearExportBundle();
        
        clearErrorMessage();
        
        mObjectPicker.redoSearch( false );
        
        updateVisible();
        }
    else if( inTarget == &mExportTagField ) {
        updateVisible();
        }
    else if( inTarget == &mCopyHashButton ) {
        if( mCurrentHash != NULL ) {
            setClipboardText( mCurrentHash );
            }
        clearErrorMessage();
        }
    else if( inTarget == &mCopyIDListButton ) {
        SimpleVector<char> listTextWorking;
        
        SimpleVector<int> *currentList = getCurrentExportList();
        
        for( int i=0; i< currentList->size(); i++ ) {
            
            if( i > 0 ) {
                listTextWorking.push_back( '\n' );
                }
            
            char *idString = autoSprintf( "%d", 
                                          currentList->getElementDirect( i ) );
            
            listTextWorking.appendElementString( idString );
            delete [] idString;
            }
        char *listText = listTextWorking.getElementString();
        
        setClipboardText( listText );
        
        delete [] listText;
        
        clearErrorMessage();
        }
    else if( inTarget == &mPasteIDListButton ) {
        clearErrorMessage();
        
        char *listText = getClipboardText();

        char someNotFound = false;
        char oneScanSuccess = false;
        
        
        if( listText != NULL ) {
            int oldListLen = getCurrentExportList()->size();
            
            char success;
            char *nextIntToScan = listText;
            
            int scannedInt = scanIntAndSkip( &nextIntToScan, &success );
            
            while( success ) {
                oneScanSuccess = true;
                
                ObjectRecord *o = getObject( scannedInt, true );
                
                if( o != NULL ) {
                    addExportObject( scannedInt );
                    }
                else {
                    someNotFound = true;
                    }
                
                scannedInt = scanIntAndSkip( &nextIntToScan, &success );
                }
            
            delete [] listText;
            
            
            
            int newListLen = getCurrentExportList()->size();
            
            if( newListLen > oldListLen ) {
                
                // objects added by paste always end up at the end of the
                // list
                mSelectionIndex = newListLen - 1;
                
                mObjectPicker.redoSearch( false );
                
                updateVisible();
                }
            

            if( someNotFound ) {
                setErrorMessage( "Some pasted object IDs not found." );
                }
            else if( ! oneScanSuccess ) {
                setErrorMessage( "No object IDs found in clipboard text." );
                }
            else if( newListLen == oldListLen ) {
                setErrorMessage( "Pasted text added no additional object IDs." );
                }
            }
        else {
            setErrorMessage( "Failed to get clipboard text." );
            }
        }
    }



void EditorExportPage::updateVisible() {
    char listNotEmpty = ( getCurrentExportList()->size() > 0 );
    
    mExportTagField.setVisible( listNotEmpty );
    mClearButton.setVisible( listNotEmpty );
    
    char tagNotEmpty = false;
    
    char *tagText = mExportTagField.getText();
    
    if( strlen( tagText ) != 0 ) {
        tagNotEmpty = true;
        }
    
    delete [] tagText;

    mExportButton.setVisible( tagNotEmpty && listNotEmpty );

    if( mCurrentHash != NULL ) {
        delete [] mCurrentHash;
        mCurrentHash = NULL;
        }
    
    mCopyHashButton.setVisible( false );
    mCopyIDListButton.setVisible( false );
    mPasteIDListButton.setVisible( false );

    if( listNotEmpty ) {
        mCurrentHash = getCurrentBundleHash();
    
        if( mCurrentHash != NULL ) {
            
            if( isClipboardSupported() ) {
                mCopyHashButton.setVisible( true );
                }
            }
        
        if( isClipboardSupported() ) {
            mCopyIDListButton.setVisible( true );
            }
        }
    
    if( isClipboardSupported() ) {
        mPasteIDListButton.setVisible( true );
        }
    }



static void drawObjectList( SimpleVector<int> *inList,
                            int inSelectionIndex = -1 ) {

    int num = inList->size();
        
    FloatColor boxColor = { 0.25f, 0.25f, 0.25f, 1.0f };
    
    FloatColor altBoxColor = { 0.35f, 0.35f, 0.35f, 1.0f };
    
    FloatColor selBoxColor = { 0.35f, 0.35f, 0.0f, 1.0f };

    doublePair pos;
    
    pos.x = -50;
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
    
    SimpleVector<int> *currentList = getCurrentExportList();
    
    if( currentList->size() > 0 ) {
        
        
        // draw object under other stuff
        
        int selectedID = currentList->getElementDirect( mSelectionIndex );
        
        doublePair objDisplayPos = mClearButton.getPosition();
        
        objDisplayPos.y += 300;
        
        setDrawColor( 0.75, 0.75, 0.75, 1 );
        
        drawRect( objDisplayPos, 150, 200 );

        objDisplayPos.y -= 200;
        objDisplayPos.y += 64;

        ObjectRecord *o = getObject( selectedID );
        
        drawObject( o, 2,
                    objDisplayPos, 
                    0, false, false, 20,
                    // 1 for front arm, -1 for back arm, 0 for no hiding
                    0,
                    false,
                    false,
                    getEmptyClothingSet() );

        

        doublePair legendPos = mObjectEditorButton.getPosition();
        
        legendPos.x = -350;
        legendPos.y += 50;

        
        if( ! TextField::isAnyFocused() ) {
            drawKeyLegend( &mKeyLegend, legendPos );
            }
        
        drawObjectList( getCurrentExportList(), mSelectionIndex );
    
        if( mCurrentHash != NULL ) {
            doublePair hashPos = mExportButton.getPosition();
        
            hashPos.x += 100;
        
            char *hashString = autoSprintf( "Hash: %s", mCurrentHash );
        
            setDrawColor( 1, 1, 1, 1 );

            smallFontFixed->drawString( hashString, hashPos, alignLeft );
        
            delete [] hashString;
            }
        }

    if( mCurrentErrorMessage != NULL ) {
        doublePair messagePos = { 0, -330 };
        drawMessage( mCurrentErrorMessage, messagePos, true );
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
    
    if( inX > -200 && inX < 100 &&
        inY > -225 && inY < 225 ) {
        // click on list, make keyboard active on list again
        
        TextField::unfocusAll();
        }
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
            
            updateVisible();
            }
        }    
    }



void EditorExportPage::specialKeyDown( int inKeyCode ) {
    if( TextField::isAnyFocused() ) {
        return;
        }

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



void EditorExportPage::clearErrorMessage() {
    if( mCurrentErrorMessage != NULL ) {
        delete [] mCurrentErrorMessage;
        }
    mCurrentErrorMessage = NULL;
    }



void EditorExportPage::setErrorMessage( const char *inMessage ) {
    clearErrorMessage();
    mCurrentErrorMessage = stringDuplicate( inMessage );
    }

