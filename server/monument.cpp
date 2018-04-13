#include <stdio.h>



extern char monumentCallPending;
extern int monumentCallX, monumentCallY, monumentCallID;



void monumentAction( int inX, int inY, int inObjectID, int inPlayerID,
                     int inAction ) {
    printf( "Monument action happened.\n" );
    
    if( inAction == 3 ) {
        monumentCallPending = true;
        monumentCallX = inX;
        monumentCallY = inY;
        monumentCallID = inObjectID;
        }
    }

