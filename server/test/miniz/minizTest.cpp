#include "minorGems/formats/encodingUtils.h"

#include <stdio.h>


int main() {
    int dataLength = 4061;
    
    unsigned char *data = new unsigned char[dataLength];
    

    FILE *f = fopen( "badMiniZData.txt", "r" );
    
    if( f != NULL ) {
        
        int numRead = fread( data, 1, dataLength, f );
        

        fclose( f );
        

        if( numRead == dataLength ) {
            
            for( int t=0; t<1; t++ ) {
                    
                int compressedSize;
                unsigned char *compressedChunkData =
                    zipCompress( data, dataLength,
                                 &compressedSize );
                
                int numZeroBytes = 0;
                for( int c = 0; c<compressedSize; c++ ) {
                    if( compressedChunkData[c] == 0 ) {
                        numZeroBytes ++;
                        }
                    }
                if( false )printf( "%d zero bytes in compressed data out of %d bytes\n",
                        numZeroBytes, compressedSize );
                
                FILE *out = fopen( "outTestOld.zip", "w" );
                fwrite( compressedChunkData, 1, compressedSize, out );
                fclose( out );

                if( compressedChunkData != NULL ) {
                    
                    unsigned char *decompData =
                        zipDecompress( compressedChunkData, 
                                       compressedSize,
                                       dataLength );
                    
                    if( decompData != NULL ) {
                        char mismatch = false;
                        for( int i=0; i<dataLength; i++ ) {
                            if( data[i] != decompData[i] ) {
                                mismatch = true;
                                printf( "Decomp mismatch at byte %d\n",
                                        i );
                                }
                            }
                        
                        if( ! mismatch ) {
                            //printf( "Perfect decompress match\n" );
                            }
                        
                        delete [] decompData;
                        }
                    
                    
                    delete [] compressedChunkData;
                    }
                }
            }
        }
    
    delete [] data;
    


    return 1;
    }
