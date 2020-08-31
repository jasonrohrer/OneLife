#include "minorGems/util/SettingsManager.h"
#include "minorGems/io/file/File.h"
#include "minorGems/util/stringUtils.h"

#include <stdlib.h>


void getEveMovingGridPosition( int *inOutX, int *inOutY, char inSave ) {
    int oldX = *inOutX;
    int oldY = *inOutY;

    int rows = SettingsManager::getIntSetting( "eveMovingGridRows", 5 );

    int jump = SettingsManager::getIntSetting( "eveMovingGridJump", 320 );
    

    int newX = oldX;
    int newY = oldY;

    // moves down (negative y) on even rows, up on odd rows
    int lastDir = ( abs( oldX ) / jump ) % 2;

    if( lastDir == 0 ) {
        lastDir = -1;
        }

    int lastRow = abs( oldY ) / jump;


    if( lastRow >= rows ) {
        // out of bounds
        // force-move it back in bounds
        newY = 0;
        }
    else if( lastDir == -1 && lastRow == rows - 1 ) {
        // moving down before, and in bottom row

        // go to next column
        newX -= jump;
        }
    else if( lastDir == 1 && lastRow == 0 ) {
        // moving up before, and in top row
    
        // go to next column
        newX -= jump;
        }
    else {
        // keep moving in same colum and same dir
        newY += lastDir * jump;
        }    
    

    if( inSave ) {
        File eveLocFile( NULL, "lastEveLocation.txt" );
        char *locString = autoSprintf( "%d,%d", newX, newY );
        eveLocFile.writeToFile( locString );
        delete [] locString;
        }
    
    *inOutX = newX;
    *inOutY = newY;
    }
