#include <stdlib.h>
#include <stdio.h>

#include "minorGems/io/file/File.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SimpleVector.h"



void usage() {

    printf( "\n\nUsage:\n\n" );

    printf( "repairSprites goodSpritesDir badSpritesDir\n\n\n" );
    
    exit( 1 );
    }




int main( int inNumArgs, const char **inArgs ) {
    if( inNumArgs != 3 ) {
        usage();
        }
    
    const char *goodPath = inArgs[1];
    
    const char *badPath = inArgs[2];
    
    File goodDir( NULL, goodPath );

    File badDir( NULL, badPath );
    
    
    if( ! goodDir.exists() ||
        ! goodDir.isDirectory() ||
        ! badDir.exists() ||
        ! badDir.isDirectory() ) {
        
        printf( "\n\nError:\n"
                "Both good and bad sprite directories must exist\n" );
        
        usage();
        }
    
    
    int numGoodFiles;
    File **goodFiles = goodDir.getChildFiles( &numGoodFiles );
    

    int numBadFiles;
    File **badFiles = badDir.getChildFiles( &numBadFiles );    

    int totalBytesReplaced = 0;
    
    for( int i=0; i<numBadFiles; i++ ) {
        File *f = badFiles[i];
        
        char *name = f->getFileName();
        
        if( stringEndsWith( name, ".tga" ) ) {
            
            int badID = -1;
            sscanf( name, "%d.tga", &badID );


            char found;
            char *txtName = replaceOnce( name, ".tga", ".txt", &found );
            
            File *txtFile = badDir.getChildFile( txtName );
            
            delete [] txtName;

            char *txtContents = txtFile->readFileContents();

            char *tmp = trimWhitespace( txtContents );
            delete [] txtContents;
            txtContents = tmp;
            

            delete txtFile;

            // find all txt files in goodDir that match
            // save their ID numbers

            SimpleVector<int> goodMatchingIDs;
            
            for( int j=0; j<numGoodFiles; j++ ) {
                File *goodF = goodFiles[j];
        
                char *goodName = goodF->getFileName();
                
                if( stringEndsWith( goodName, ".txt" ) ) {
                    
                    
                    char *goodTxtContents = goodF->readFileContents();

                    tmp = trimWhitespace( goodTxtContents );
                    delete [] goodTxtContents;
                    goodTxtContents = tmp;
            

                    if( strcmp( goodTxtContents, txtContents ) == 0 ) {
                        // match

                        int id = -1;
                        sscanf( goodName, "%d.txt", &id );
                        
                        if( id != -1 ) {
                            goodMatchingIDs.push_back( id );
                            }
                        }
                    delete [] goodTxtContents;
                    }
                
                delete [] goodName;
                }
            
            delete [] txtContents;

            if( goodMatchingIDs.size() == 0 ) {
                printf( "Bad TGA file %d.tga matches with nothing "
                        "in good dir?\n", badID );
                }
            
            /*
            printf( "%s matches with:\n", name );
            for( int d=0; d < goodMatchingIDs.size(); d++ ) {
                printf( "   %d", goodMatchingIDs.getElementDirect( d ) );
                }
            */

            int closestMatchID = -1;
            float closestFractionDifferent = 1.0f;
            
            int badTGALength = -1;
            unsigned char *badTGAData = f->readFileContents( &badTGALength );

            for( int d=0; d < goodMatchingIDs.size(); d++ ) {
                int id = goodMatchingIDs.getElementDirect( d );
                
                char *goodTGAName = autoSprintf( "%d.tga", id );
                
                File *goodTGAFile = goodDir.getChildFile( goodTGAName );
                
                delete [] goodTGAName;

                int goodTGALength = -1;
                
                unsigned char *goodTGAData = 
                    goodTGAFile->readFileContents( &goodTGALength );
                
                delete goodTGAFile;

                int shortLenth = goodTGALength;
                if( shortLenth > badTGALength ) {
                    shortLenth = badTGALength;
                    }
                
                int numBytesChanged = 0;
                
                for( int b=0; b<shortLenth; b++ ) {
                    if( badTGAData[b] != goodTGAData[b] ) {
                        numBytesChanged++;
                        }
                    }
                
                delete [] goodTGAData;

                int missingBytes = 0;
                if( shortLenth < goodTGALength ) {
                    missingBytes = goodTGALength - shortLenth;
                    }
                else if( shortLenth < badTGALength ) {
                    missingBytes = badTGALength - shortLenth;
                    }

                numBytesChanged += missingBytes;

                float fractionChanged = (float)numBytesChanged / shortLenth;
                
                if( fractionChanged < closestFractionDifferent ) {
                    closestFractionDifferent = fractionChanged;
                    closestMatchID = id;
                    }
                }
            
            /*
              printf( "  Closest: %d (%f)\n", 
                    closestMatchID, closestFractionDifferent );
            */

            if( closestFractionDifferent > 0 ) {
                printf( "Found mismatch in closest TGA data.  "
                        "Overwriting bad %d.tga with good data from %d.tga\n",
                        badID, closestMatchID );


                char *goodTGAName = autoSprintf( "%d.tga", closestMatchID );
                
                File *goodTGAFile = goodDir.getChildFile( goodTGAName );
                
                delete [] goodTGAName;
                
                totalBytesReplaced += goodTGAFile->getLength();
                
                goodTGAFile->copy( f );

                delete goodTGAFile;
                }

            delete [] badTGAData;
            }
        
            
        
        delete [] name;
        }
    
    
    printf( "Total bytes replaced = %d\n", totalBytesReplaced );


    for( int i=0; i<numGoodFiles; i++ ) {
        delete goodFiles[i];
        }
    delete [] goodFiles;


    for( int i=0; i<numBadFiles; i++ ) {
        delete badFiles[i];
        }
    delete [] badFiles;




    return 0;
    }
