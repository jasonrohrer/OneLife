#include <stdio.h>

#include "minorGems/graphics/Image.h"

#include "minorGems/io/file/File.h"
#include "minorGems/io/file/FileInputStream.h"
#include "minorGems/io/file/FileOutputStream.h"
#include "minorGems/graphics/converters/TGAImageConverter.h"

#include "minorGems/util/stringUtils.h"


static Image *expandToPowersOfTwo( Image *inImage ) {
    
    int w = 1;
    int h = 1;
                    
    while( w < inImage->getWidth() ) {
        w *= 2;
        }
    while( h < inImage->getHeight() ) {
        h *= 2;
        }
    
    return inImage->expandImage( w, h );
    }



int main() {
    File spritesDir( NULL, "sprites" );
    
    if( ! spritesDir.isDirectory() ) {
        printf( "sprites directory not found\n" );
        return 1;
        }
    
    int numFiles;
    File **files = spritesDir.getChildFiles( &numFiles );

    for( int i=0; i<numFiles; i++ ) {
        
        char *name = files[i]->getFileName();
        
        if( strstr( name, ".tga" ) != NULL ) {
            printf( "Processing %s\n", name );
            
            int id = -1;
            
            sscanf( name, "%d.tga", &( id ) );

            if( id != -1 ) {
                char *infoFileName = autoSprintf( "%d.txt", id );
                
                File *infoFile = spritesDir.getChildFile( infoFileName );
                
                if( infoFile != NULL ) {

                    char *contents = infoFile->readFileContents();
                    
                    SimpleVector<char *> *tokens = tokenizeString( contents );
                    int numTokens = tokens->size();
            
                    if( numTokens <= 2 ) {
                        // no center specified
                        TGAImageConverter conv;
                        
                        int mult = 0;
                        sscanf( tokens->getElementDirect( 1 ),
                                "%d", &mult );
                        
                        
                        FileInputStream *inStream = 
                            new FileInputStream( files[i] );
                        
                        Image *image = conv.deformatImage( inStream );
                        
                        delete inStream;

                        int w = image->getWidth();
                        int h = image->getHeight();
                        printf( "  Old w,h = %d, %d\n", w, h );
                        
                        int centerX = w /2;
                        int centerY = h /2;
                        
                        double *a = image->getChannel( 3 );
                        
                        int firstX = w;
                        int lastX = -1;
    
                        int firstY = h;
                        int lastY = -1;
                        
                        for( int y=0; y<h; y++ ) {
                            for( int x=0; x<w; x++ ) {
                                int i = y * w + x;
                                
                                if( a[i] > 0.0 ) {
                
                                    if( x < firstX ) {
                                        firstX = x;
                                        }
                                    if( x > lastX ) {
                                        lastX = x;
                                        }
                                    if( y < firstY ) {
                                        firstY = y;
                                        }
                                    if( y > lastY ) {
                                        lastY = y;
                                        }
                                    }
                                }
                            }
    
                        Image *trimmedImage = NULL;
    
                        if( lastX > firstX && lastY > firstY ) {
        
                            trimmedImage = 
                                image->getSubImage( firstX, firstY, 
                                                    1 + lastX - firstX, 
                                                    1 + lastY - firstY );
                            }
                        else {
                            // no non-trans areas, don't trim it
                            trimmedImage = image->copy();
                            }
    
                        
                        // make relative to new trim
                        centerX -= firstX;
                        centerY -= firstY;
                        

                        w = trimmedImage->getWidth();
                        h = trimmedImage->getHeight();
                        
                        printf( "  trimmed w,h = %d, %d\n", w, h );

                        Image *trimmedExpandedImage = 
                            expandToPowersOfTwo( trimmedImage );
                        
                        int newW = trimmedExpandedImage->getWidth();
                        int newH = trimmedExpandedImage->getHeight();
                        printf( "  expanded w,h = %d, %d\n", newW, newH );

                        
                        if( mult ) {
                            // make trans areas white (black border
                            // after expansion
                            double *r = trimmedExpandedImage->getChannel( 0 );
                            double *g = trimmedExpandedImage->getChannel( 1 );
                            double *b = trimmedExpandedImage->getChannel( 2 );
                            double *a = trimmedExpandedImage->getChannel( 3 );

                            int numPixels = newW * newH;
                            
        
                            for( int i=0; i<numPixels; i++ ) {
                                if( a[i] == 0 ) {
                                    r[i] =  1;
                                    g[i] =  1;
                                    b[i] =  1;
                                    }
                                }
                            }

                        
                        centerX += ( newW - w ) / 2;
                        centerY += ( newH - h ) / 2;

                        printf( "  new center = %d, %d\n", centerX, centerY );

                        delete trimmedImage;


                        FileOutputStream *outStream = 
                            new FileOutputStream( files[i] );

                        conv.formatImage( trimmedExpandedImage, outStream );
                        
                        int centerOffsetX = centerX - (newW/2);
                        int centerOffsetY = centerY - (newH/2);
                        
                        char *newContents = autoSprintf( "%s %d %d",
                                                         contents,
                                                         centerOffsetX,
                                                         centerOffsetY );

                        infoFile->writeToFile( newContents );
                        
                        delete [] newContents;
                        
                        delete image;
                        }
                    else {
                        // else custom center already exists, leave it alone
                        printf( "  Custom center already exists\n" );
                        }
                    

                    tokens->deallocateStringElements();
                    delete tokens;


                    delete [] contents;

                    delete infoFile;
                    }
                delete [] infoFileName;
                }
            
            }
        

        delete [] name;

        delete files[i];
        }
    
    delete [] files;
    }
