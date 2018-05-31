#include <stdio.h>
#include "minorGems/io/file/File.h"

#include "../gameSource/objectBank.h"


extern char monumentCallPending;
extern int monumentCallX, monumentCallY, monumentCallID;



char *getMonumentLogFilePath( int inX, int inY, const char *inExtra = "" ) {
        
    File monumentDir( NULL, "monumentLogs" );
    
    char *returnVal = NULL;
    
    
    if( ! monumentDir.exists() ) {
        monumentDir.makeDirectory();
        }
    if( monumentDir.isDirectory() ) {
        char *name = autoSprintf( "%d_%d%s.txt", inX, inY, inExtra );

        File *child = monumentDir.getChildFile( name );
        delete [] name;
        
        if( child != NULL ) {
            returnVal = child->getFullFileName();            
            delete child;
            }
        }

    return returnVal;
    }



FILE *getMonumentLogFile( int inX, int inY ) {
    char *path = getMonumentLogFilePath( inX, inY );
    
    if( path != NULL ) {
        
        FILE *f = fopen( path, "a" );
        delete [] path;
        return f;
        }
    return NULL;
    }



void finalizeMonumentLogFile( int inX, int inY ) {
    char *pathOld = getMonumentLogFilePath( inX, inY );

    double t = Time::getCurrentTime();
    
    char *extra = autoSprintf( "_done_%.0f", t );
    
    char *pathNew = getMonumentLogFilePath( inX, inY, extra );

    rename( pathOld, pathNew );

    delete [] pathOld;
    delete [] pathNew;
    delete [] extra;
    }



// defined in server.cpp
char *getPlayerName( int inID );


void monumentAction( int inX, int inY, int inObjectID, int inPlayerID,
                     int inAction ) {
    printf( "Monument action happened.\n" );
    
    if( inAction == 3 ) {
        monumentCallPending = true;
        monumentCallX = inX;
        monumentCallY = inY;
        monumentCallID = inObjectID;
        }
    else {
        // log it
        FILE *f = getMonumentLogFile( inX, inY );
        
        if( f != NULL ) {
            const char *name = getPlayerName( inPlayerID );
            if( name == NULL ) {
                name = "NAMELESS";
                }
            
            fprintf( f, "%d: %d %d %d %d %s time_%.f\n", 
                     inAction, inX, inY, inObjectID, inPlayerID, name,
                     Time::getCurrentTime() );
            
            if( inAction == 2 ) {
                // monument done
                ObjectRecord *o = getObject( inObjectID );
                
                if( o != NULL ) {
                    fprintf( f, "%s\n", o->description );
                    }
                }
            
            fclose( f );
            }

        if( inAction == 2 ) {
            finalizeMonumentLogFile( inX, inY );
            }
        }
    
    }

