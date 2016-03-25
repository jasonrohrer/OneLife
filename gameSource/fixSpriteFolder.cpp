// compile with:
// g++ -o fixSpriteFolder fixSpriteFolder.cpp -I../.. ../../minorGems/util/stringUtils.o ../../minorGems/io/file/linux/PathLinux.o



#include "minorGems/util/stringUtils.h"

#include "minorGems/io/file/File.h"


int main() {
    File spritesDir( NULL, "sprites" );
    
    if( spritesDir.exists() && spritesDir.isDirectory() ) {
        
        int numFiles;
        File **childFiles = spritesDir.getChildFiles( &numFiles );


        for( int i=0; i<numFiles; i++ ) {
            
            if( childFiles[i]->isDirectory() ) {
                
                char *tag = childFiles[i]->getFileName();

                int numTGAFiles;
                File **tgaFiles = childFiles[i]->getChildFiles( &numTGAFiles );
                
                for( int t=0; t<numTGAFiles; t++ ) {
                    
                    if( ! tgaFiles[t]->isDirectory() ) {
                        
                        char *tgaFileName = tgaFiles[t]->getFileName();
                        
                        if( strstr( tgaFileName, ".tga" ) != NULL ) {
                            
                            // a tga file!
                            
                            int id = 0;
                            char mult = false;
                            if( tgaFileName[0] == 'm' ) {
                                mult = true;
                                sscanf( tgaFileName, "m%d.tga", &id );
                                }
                            else {
                                mult = false;
                                sscanf( tgaFileName, "%d.tga", &id );
                                }
                                
                            printf( "Scanned id = %d\n", id );

                            int tgaDataLength;
                            
                            unsigned char *tgaData =
                                tgaFiles[t]->readFileContents( 
                                    &tgaDataLength );

                            tgaFiles[t]->remove();
                            
                            // FIXME
                            char *newTGAName = autoSprintf( "%d.tga", id );
                            char *newTXTName = autoSprintf( "%d.txt", id );
                            
                            File *newTGAFile = 
                                spritesDir.getChildFile( newTGAName );
                            
                            File *newTXTFile = 
                                spritesDir.getChildFile( newTXTName );
                            
                            delete [] newTGAName;
                            delete [] newTXTName;

                            // write data
                            newTGAFile->writeToFile( tgaData, tgaDataLength );
                            
                            // write text contents (tag and mult flag)
                            char *txtData = autoSprintf( "%s %d",
                                                         tag, mult );
                            
                            newTXTFile->writeToFile( txtData );
                            
                            delete [] txtData;
                            delete [] tgaData;

                            delete newTGAFile;
                            delete newTXTFile;
                            }
                                
                        delete [] tgaFileName;
                        }
                    
                    delete tgaFiles[t];
                    }
                
                delete [] tag;
                
                delete [] tgaFiles;
            
                childFiles[i]->remove();
                }
            
            delete childFiles[i];
            }
        
        delete [] childFiles;
        }
    }
