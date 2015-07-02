#include "SpritePicker.h"

#include "spriteBank.h"

#include "minorGems/game/Font.h"
#include "minorGems/game/drawUtils.h"

#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"

#include "minorGems/graphics/converters/PNGImageConverter.h"
#include "minorGems/graphics/converters/TGAImageConverter.h"

#include "minorGems/io/file/FileInputStream.h"



#define PER_PAGE 5

extern Font *mainFont;
extern Font *smallFont;


SpritePicker::SpritePicker( double inX, double inY )
        : PageComponent( inX, inY ), 
          mSkip( 0 ),
          mResults( NULL ),
          mNumResults( 0 ),
          mNextButton( smallFont, +60, -290, "Next" ), 
          mPrevButton( smallFont, -60, -290, "Prev" ), 
          mSearchField( mainFont, 
                        0,  100, 4,
                        false,
                        "", NULL, " " ),
          mSelectionIndex( -1 ) {

    addComponent( &mNextButton );
    addComponent( &mPrevButton );
    addComponent( &mSearchField );
    
    mNextButton.addActionListener( this );
    mPrevButton.addActionListener( this );
    mSearchField.addActionListener( this );
    
    mSearchField.setFireOnAnyTextChange( true );

    redoSearch();
    }



SpritePicker::~SpritePicker() {
    if( mResults != NULL ) {
        delete [] mResults;
        }
    }



void SpritePicker::redoSearch() {
    char *search = mSearchField.getText();
    
    if( mResults != NULL ) {
        delete [] mResults;
        mResults = NULL;
        }

    int numRemain;
    
    mResults = searchSprites( search, 
                              mSkip, 
                              PER_PAGE, 
                              &mNumResults, &numRemain );

    
    mPrevButton.setVisible( mSkip > 0 );
    mNextButton.setVisible( numRemain > 0 );
    
    delete [] search;
    mSelectionIndex = -1;
    }



        
void SpritePicker::actionPerformed( GUIComponent *inTarget ) {
    if( inTarget == &mNextButton ) {
        mSkip += PER_PAGE;
        redoSearch();
        }
    else if( inTarget == &mPrevButton ) {
        mSkip -= PER_PAGE;
        if( mSkip < 0 ) {
            mSkip = 0;
            }
        redoSearch();
        }
    else if( inTarget == &mSearchField ) {
        mSkip = 0;
        redoSearch();
        }
    }




void SpritePicker::draw() {
    setDrawColor( 0.75, 0.75, 0.75, 1 );
    
    doublePair bgPos = { 0, -85 };
    
    drawRect( bgPos, 80, 160 );
    


    if( mResults != NULL ) {
        doublePair pos = { -50, 40 };
        
        
        for( int i=0; i<mNumResults; i++ ) {
            if( i == mSelectionIndex ) {
                setDrawColor( 1, 1, 1, 1 );
                doublePair selPos = pos;
                selPos.x = 0;
                drawRect( selPos, 80, 32 );
                }

            setDrawColor( 1, 1, 1, 1 );
            drawSprite( mResults[i]->sprite, pos );
            
            doublePair textPos = pos;
            textPos.x += 52;
            
            setDrawColor( 0, 0, 0, 1 );
            smallFont->drawString( mResults[i]->tag, textPos, alignLeft );
            

            pos.y -= 64;
            }
        }
    
    }

        

void SpritePicker::pointerUp( float inX, float inY ) {
    if( inX > -80 && inX < 80 ) {
        
        inY -= 40;
        
        inY *= -1;
        
        inY += 32;
        
        mSelectionIndex = (int)( inY / 64 );
        
        if( mSelectionIndex >= PER_PAGE ) {
            mSelectionIndex = -1;
            }
        if( mSelectionIndex < 0 ) {
            mSelectionIndex = -1;
            }
        }
    }



int SpritePicker::getSelectedSprite() {
    if( mSelectionIndex == -1 ) {
        return -1;
        }
    
    if( mSelectionIndex >= mNumResults ) {
        return -1;
        }
    
    return mResults[mSelectionIndex]->id;
    }
