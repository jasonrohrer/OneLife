#include <stdio.h>
#include <stdlib.h>

#include "minorGems/util/SimpleVector.h"
#include "minorGems/util/stringUtils.h"


void usage() {
    printf( "\nUsage Example:\n\n" );
    printf( "printLiving lifeLog/2019_09September_26_Thursday.txt\n\n" );
    exit( 1 );
    }



void stringRemove( SimpleVector<char*> *inVec, char *inString ) {
    for( int i=0; i< inVec->size(); i++ ) {
        char *s = inVec->getElementDirect( i );
        
        if( strcmp( s, inString ) == 0 ) {
            delete [] s;
            inVec->deleteElement( i );
            break;
            }
        }
    }



// insert only if unique
void stringInsert( SimpleVector<char*> *inVec, char *inString ) {
    stringRemove( inVec, inString );
    
    inVec->push_back( stringDuplicate( inString ) );
    }





int main( int inNumArgs, char **inArgs ) {
    
    if( inNumArgs != 2 ) {
        usage();
        }
    
    FILE *f = fopen( inArgs[1], "r" );
    
    if( f == NULL ) {
        usage();
        }
    
    /*
      sample log entries

      B 1569456046 2101582 matt_tommasi@hotmail.co.nz F (237,-321) parent=2101529,tallywawa_1234@hotmail.com pop=74 chain=79
      D 1569456076 2101436 76561198248876564@steamgames.com age=50.16 M (-236,288) hunger pop=73
    */

    SimpleVector<char*> liveEmails;
    
    while( ! feof( f ) ) {
        
        // read to end of line
        while( ! feof( f ) && fgetc( f ) != '\n' ) {
            
            }

        if( ! feof( f ) ) {
            char c = fgetc( f );
            
            double t;
            int id;

            char email[200];
            
            int numRead = fscanf( f, " %lf %d %199s ", &t, &id, email );
            
            if( numRead == 3 ) {
                
                if( c == 'D' ) {
                    // died, remove from list
                    
                    stringRemove( &liveEmails, email );
                    }
                else if( c == 'B' ) {
                    stringInsert( &liveEmails, email );
                    }
                }
            }
        }
    
    fclose( f );
    
    for( int i=0; i<liveEmails.size(); i++ ) {
        printf( "%s\n", liveEmails.getElementDirect( i ) );
        }
    
    liveEmails.deallocateStringElements();



    return 1;
    }
