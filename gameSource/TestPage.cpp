#include "TestPage.h"

#include "minorGems/game/drawUtils.h"
#include "minorGems/game/game.h"


TestPage::TestPage() 
        : mSpriteA( loadSpriteBase( "sprites/77.tga" , false ) ),
          mSpriteABlank( loadSpriteBase( "77_78_blank.tga", false ) ),
          mSpriteB( loadSpriteBase( "sprites/78.tga", false ) ),
          mSpriteC( loadSpriteBase( "77_78.tga", false ) ),
          mSpriteD( loadSpriteBase( "77_78_crop.tga", false ) ) {

    //toggleTransparentCropping( true );
    
    mSpriteDAutoCrop = loadSpriteBase( "77_78_crop.tga", false );

    //toggleTransparentCropping( false );
    
    Image *sprite78 = readTGAFileBase( "sprites/78.tga" );
    
    int numPixels = sprite78->getWidth() * sprite78->getHeight();
    
    double *alpha = sprite78->getChannel( 3 );
    
    unsigned char *alphaBytes = new unsigned char[ numPixels ];
    
    for( int i=0; i<numPixels; i++ ) {
        alphaBytes[i] = lrint( 255 * alpha[ i ] );
        }

    mSpriteB_bw = fillSpriteAlphaOnly( alphaBytes,
                                       sprite78->getWidth(),
                                       sprite78->getHeight() );
    delete sprite78;
    delete [] alphaBytes;
    }

    

TestPage::~TestPage() {
    freeSprite( mSpriteA );
    freeSprite( mSpriteABlank );
    freeSprite( mSpriteB );
    freeSprite( mSpriteC );
    freeSprite( mSpriteD );
    freeSprite( mSpriteB_bw );
    }


int toggle = 0;
int yOffset = 0;
int xOffset = 0;

int numToDraw = 100;


void TestPage::pointerDown( float inX, float inY ) {
    xOffset = lrint( inX );
    yOffset = lrint( inY );
    }


void TestPage::keyDown( unsigned char inASCII ) {
    if( inASCII == 't' ) {
        toggle = ( toggle + 1 ) % 7;
        }
    else if( inASCII == 'p' ) {
        numToDraw += 10;
        printf( "Drawing %d objects\n",
                numToDraw );
        }
    else if( inASCII == 'o' ) {
        numToDraw -= 10;
        printf( "Drawing %d objects\n",
                numToDraw );
        }
    else if( inASCII == 'P' ) {
        numToDraw += 1;
        printf( "Drawing %d objects\n",
                numToDraw );
        }
    else if( inASCII == 'O' ) {
        numToDraw -= 1;
        printf( "Drawing %d objects\n",
                numToDraw );
        }
    
    }

        

void TestPage::draw( doublePair inViewCenter, 
                     double inViewSize ) {
    
    double startTime = game_getCurrentTime();
    
    
    setDrawColor( 1, 1, 1, 1 );
    

    drawSquare( inViewCenter, 600 );
    
    doublePair pos = inViewCenter;
    //pos.x -= 300;
    
    pos.x = xOffset;
    pos.y = yOffset;

    if( toggle == 2 || toggle == 3 ) {
        pos.y += 178;
        }
    

    for( int i=0; i<numToDraw; i++ ) {
        
        switch( toggle ) {
            case 0:
                drawSprite( mSpriteA, pos );
                drawSprite( mSpriteB, pos );
                break;
            case 1:
                drawSprite( mSpriteC, pos );
                break;
            case 2:
                drawSprite( mSpriteD, pos );
                break;
            case 3:
                drawSprite( mSpriteDAutoCrop, pos );
                break;
            case 4:
                drawSprite( mSpriteABlank, pos );
                break;
            case 5:
                drawSprite( mSpriteB, pos );
                break;
            case 6:
                drawSprite( mSpriteB_bw, pos );
                break;
            }
        
        //pos.x += 5;
        }
    
    //printf( "Time to draw %d trees = %f ms\n",
    //        numToDraw, 1000 * ( game_getCurrentTime() - startTime ) );
    }

