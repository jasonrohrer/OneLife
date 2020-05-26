#include <stdio.h>
#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"


typedef struct EmailRecord {
        char *email;
        double lifeMinutes;
        int curses;
    } EmailRecord;


static SimpleVector<EmailRecord> records;


EmailRecord *getRecord( char *inEmail, char inMakeNew = true ) {
    
    for( int i=0; i<records.size(); i++ ) {
        EmailRecord *r = records.getElement( i );
        
        if( strcmp( r->email, inEmail ) == 0 ) {
            return r;
            }
        }

    if( ! inMakeNew ) {
        return NULL;
        }
    
    EmailRecord r = { stringDuplicate( inEmail ), 0.0, 0 };
    
    records.push_back( r );
    
    return records.getElement( records.size() - 1 );
    }



int main() {
    FILE *curseFile = fopen( "tempCursedEmails.txt", "r" );

    FILE *livesFile = fopen( "tempEmailLives.txt", "r" );


    if( curseFile == NULL || livesFile == NULL ) {
        printf( "Needed files not found\n" );
        return 0;    
        }


    char email[500];
    double minutesLived = 0;
    int curses = 0;
    
    int numRead = 2;
    
    int numLives = 0;
    while( numRead == 2 ) {
    
        numRead = fscanf( livesFile, "%499s age=%lf", email, &minutesLived );
        
        if( numRead == 2 ) {
            EmailRecord *r = getRecord( email );
            r->lifeMinutes += minutesLived;
            numLives++;
            }
        }

    // printf( "Read %d lives from life file\n", numLives );


    numRead = 2;

    int numCurseListings = 0;
    while( numRead == 2 ) {
    
        numRead = fscanf( curseFile, "%d %499s", &curses, email );
        
        if( numRead == 2 ) {
            // don't make new records here
            // only count curses for people who actually played recently
            // stale records ignored
            EmailRecord *r = getRecord( email, false );
            
            if( r != NULL ) {
                r->curses += curses;
                }
            numCurseListings++;
            }
        }
    

    // printf( "Read %d curse listings from curses file\n", numCurseListings );
    

    fclose( curseFile );
    fclose( livesFile );




    
    for( int i=0; i<records.size(); i++ ) {
        EmailRecord *r = records.getElement( i );
    
        printf( "%f %d %s\n", r->lifeMinutes, r->curses, r->email );
    
        delete [] r->email;
        }
    

    return 1;
    }
