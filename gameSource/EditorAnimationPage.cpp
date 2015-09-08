#include "EditorAnimationPage.h"

#include "minorGems/game/game.h"
#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"

#include "minorGems/graphics/converters/PNGImageConverter.h"

#include "minorGems/io/file/FileInputStream.h"



extern Font *mainFont;
extern Font *smallFont;

extern double frameRateFactor;


#include "SpritePickable.h"

static SpritePickable spritePickable;


#include "ObjectPickable.h"

static ObjectPickable objectPickable;



EditorAnimationPage::EditorAnimationPage()
        : mObjectEditorButton( mainFont, 0, 260, "Objects" ),
          mObjectPicker( &objectPickable, +310, 100 ) {
    
    addComponent( &mObjectEditorButton );
    addComponent( &mObjectPicker );

    mObjectEditorButton.addActionListener( this );
    }



EditorAnimationPage::~EditorAnimationPage() {
    
    }



void EditorAnimationPage::actionPerformed( GUIComponent *inTarget ) {
    
    if( inTarget == &mObjectEditorButton ) {
        setSignal( "objectEditor" );
        }
    }




void EditorAnimationPage::draw( doublePair inViewCenter, 
                   double inViewSize ) {
    }




void EditorAnimationPage::step() {
    }




void EditorAnimationPage::makeActive( char inFresh ) {
    }





void EditorAnimationPage::pointerMove( float inX, float inY ) {
    }



void EditorAnimationPage::pointerDown( float inX, float inY ) {
    }



void EditorAnimationPage::pointerDrag( float inX, float inY ) {
    }



void EditorAnimationPage::pointerUp( float inX, float inY ) {
    }




void EditorAnimationPage::keyDown( unsigned char inASCII ) {
    }



void EditorAnimationPage::specialKeyDown( int inKeyCode ) {
    }




