#include "EditorObjectPage.h"

#include "minorGems/game/game.h"
#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"

#include "minorGems/graphics/converters/PNGImageConverter.h"

#include "minorGems/io/file/FileInputStream.h"



extern Font *mainFont;



EditorObjectPage::EditorObjectPage()
        : mDescriptionField( mainFont, 
                             0,  -260, 6,
                             false,
                             "Description", NULL, " " ),
          mSaveObjectButton( mainFont, 210, -260, "Save" ),
          mImportEditorButton( mainFont, 210, 260, "Sprites" )
    //        , mSpritePicker( -310, 100 ) 
          {

    addComponent( &mDescriptionField );
    addComponent( &mSaveObjectButton );
    addComponent( &mImportEditorButton );
    //addComponent( &mSpritePicker );

    mSaveObjectButton.addActionListener( this );
    mImportEditorButton.addActionListener( this );

    mSaveObjectButton.setVisible( false );
    }



EditorObjectPage::~EditorObjectPage() {
    }








void EditorObjectPage::actionPerformed( GUIComponent *inTarget ) {
    
    if( inTarget == &mSaveObjectButton ) {
        }
    else if( inTarget == &mImportEditorButton ) {
        setSignal( "importEditor" );
        }
    
    }

        
void EditorObjectPage::draw( doublePair inViewCenter, 
                     double inViewSize ) {
    
    }


void EditorObjectPage::step() {
    }




void EditorObjectPage::makeActive( char inFresh ) {
    if( !inFresh ) {
        return;
        }


    }


void EditorObjectPage::pointerMove( float inX, float inY ) {
    }


void EditorObjectPage::pointerDown( float inX, float inY ) {
    }



void EditorObjectPage::pointerDrag( float inX, float inY ) {
    }



void EditorObjectPage::pointerUp( float inX, float inY ) {
    }


