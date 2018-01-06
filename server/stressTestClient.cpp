

#include "minorGems/network/SocketClient.h"
#include "minorGems/system/Thread.h"
#include "minorGems/util/random/JenkinsRandomSource.h"
#include "minorGems/formats/encodingUtils.h"


JenkinsRandomSource randSource;



void usage() {
    printf( "Usage:\n" );
    printf( "stressTestClient server_address server_port email_prefix num_clients\n\n" );
    
    printf( "Example:\n" );
    printf( "stressTestClient onehouronelife.com 8005 dummy 100\n\n" );
    
    exit( 1 );
    }


typedef struct Client {
        int i;
        
        Socket *sock;
        SimpleVector<unsigned char> buffer;
        
        int skipCompressedData;
        
        char pendingCMData;
        int pendingCMCompressedSize;
        int pendingCMDecompressedSize;

        int id;
        int x, y;

        char moving;
        char dead;
        char disconnected;
        
    } Client;



// NULL if no message read
char *getNextMessage( Client *inC ) {

    if( inC->skipCompressedData > 0 && inC->buffer.size() > 0 ) {
        int numToDelete = inC->skipCompressedData;
        
        if( numToDelete > inC->buffer.size() ) {
            numToDelete = inC->buffer.size();
            }
        
        inC->buffer.deleteStartElements( numToDelete );
        inC->skipCompressedData -= numToDelete;
        }
    

    if( inC->skipCompressedData > 0 ) {
        printf( "Client %d still needs to skip %d compressed bytes\n",
                inC->i, inC->skipCompressedData );
        Thread::staticSleep( 1000 );
        
        unsigned char buffer[512];
    
        int nextReadSize = inC->skipCompressedData;
        
        if( nextReadSize > 512 ) {
            nextReadSize = 512;
            }
        int numRead = nextReadSize;
        
        while( numRead == nextReadSize && nextReadSize > 0 ) {
            nextReadSize = inC->skipCompressedData;
        
            if( nextReadSize > 512 ) {
                nextReadSize = 512;
                }

            numRead = inC->sock->receive( buffer, nextReadSize, 0 );
            
            if( numRead == -1 ) {
                inC->disconnected = true;
                }

            if( numRead > 0 ) {
                inC->skipCompressedData -= numRead;
                }
            }

        if( numRead != nextReadSize && nextReadSize > 0 ) {
            // timed out waiting for rest of compressed data
            return NULL;
            }
        }
    
    
    // read all available data
    unsigned char buffer[512];
    
    int numRead = inC->sock->receive( buffer, 512, 0 );

    if( numRead == -1 ) {
        inC->disconnected = true;
        }
    
    
    while( numRead > 0 ) {
        inC->buffer.appendArray( buffer, numRead );
        numRead = inC->sock->receive( buffer, 512, 0 );
        
        if( numRead == -1 ) {
            inC->disconnected = true;
            }
        }


    if( inC->pendingCMData ) {
        if( inC->buffer.size() >= inC->pendingCMCompressedSize ) {
            inC->pendingCMData = false;
            
            unsigned char *compressedData = 
                new unsigned char[ inC->pendingCMCompressedSize ];
            
            for( int i=0; i<inC->pendingCMCompressedSize; i++ ) {
                compressedData[i] = inC->buffer.getElementDirect( i );
                }
            inC->buffer.deleteStartElements( inC->pendingCMCompressedSize );
            
            unsigned char *decompressedMessage =
                zipDecompress( compressedData, 
                               inC->pendingCMCompressedSize,
                               inC->pendingCMDecompressedSize );

            delete [] compressedData;

            if( decompressedMessage == NULL ) {
                printf( "Decompressing CM message failed\n" );
                return NULL;
                }
            else {
                char *textMessage = 
                    new char[ inC->pendingCMDecompressedSize + 1 ];
                memcpy( textMessage, decompressedMessage,
                       inC->pendingCMDecompressedSize );
                textMessage[ inC->pendingCMDecompressedSize ] = '\0';
                
                delete [] decompressedMessage;
                
                return textMessage;
                }
            }
        else {
            // wait for more data to arrive
            return NULL;
            }
        }




    // find first terminal character #
    int index = inC->buffer.getElementIndex( '#' );
        
    if( index == -1 ) {
        return NULL;
        }
    

    char *message = new char[ index + 1 ];
    
    // all but terminal character
    for( int i=0; i<index; i++ ) {
        message[i] = (char)( inC->buffer.getElementDirect( i ) );
        }
    
    // delete from buffer, including terminal character
    inC->buffer.deleteStartElements( index + 1 );

    
    message[ index ] = '\0';
    
    if( strstr( message, "CM" ) == message ) {
        inC->pendingCMData = true;
        
        sscanf( message, "CM\n%d %d\n", 
                &( inC->pendingCMDecompressedSize ), 
                &( inC->pendingCMCompressedSize ) );


        delete [] message;
        
        return NULL;
        }
    

    return message;
    }



void parsePlayerUpdateMessage( Client *inC, char *inMessageLine ) {
    SimpleVector<char*> *tokens = tokenizeString( inMessageLine );

    //printf( "\n\nParsing PU line: %s\n\n", inMessageLine );
    
    if( tokens->size() > 16 ) {
        int id = -1;
        sscanf( tokens->getElementDirect(0), "%d", &( id ) );
    
        if( inC->id == -1 ) {
            inC->id = id;
            }
        
        if( inC->id == id ) {
            // update pos
            
            if( inC->moving ) {
                //printf( "Client %d done moving\n", inC->i );
                }
            if( strcmp( tokens->getElementDirect(14), "X" ) == 0 ) {
                // dead
                inC->dead = true;
                printf( "Client %d died with PU message:  %s\n",
                        inC->i, inMessageLine );
                }
            else {
                sscanf( tokens->getElementDirect(14), "%d", &( inC->x ) );
                sscanf( tokens->getElementDirect(15), "%d", &( inC->y ) );
                inC->moving = false;
                }
            }
        }
    
    
    tokens->deallocateStringElements();
    delete tokens;
    }





int main( int inNumArgs, char **inArgs ) {
    
    if( inNumArgs != 5 ) {
        usage();
        }
    
    char *address = inArgs[1];
    
    int port = 8005;
    sscanf( inArgs[2], "%d", &port );
    
    char *emailPrefix = inArgs[3];

    int numClients = 1;
    sscanf( inArgs[4], "%d", &numClients );

    
    Client *connections = new Client[ numClients ];
    
    // first, connect each
    int numConnected = 0;
    
    for( int i=0; i<numClients; i++ ) {
        connections[i].i = i;
        connections[i].id = -1;
        connections[i].skipCompressedData = 0;
        connections[i].pendingCMData = false;
        connections[i].moving = false;
        connections[i].dead = false;
        connections[i].disconnected = false;

        HostAddress a( stringDuplicate( address ), port );
        

        char timeout =  false;
        connections[i].sock = 
            SocketClient::connectToServer( &a, 5000, &timeout );

        if( timeout ) {
            printf( "Client %d timed out when trying to connect\n", i );
            delete connections[i].sock;
            connections[i].sock = NULL;
            continue;
            }
    
        if( connections[i].sock != NULL ) {
            numConnected ++;
            
            char *email = autoSprintf( "%s_%d@dummy.com", emailPrefix, i );

            printf( "Client %d connected, logging in with email %s\n", i,
                    email );

            
            char *message = autoSprintf( "LOGIN %s aaaa aaaa#",
                                         email );
            
            connections[i].sock->send( (unsigned char*)message, 
                                       strlen( message ), true, false );
            
            delete [] message;
            delete [] email;
            }
        else {
            printf( "Client %d failed to connect\n", i );
            }
        }

    
    // process messages
    while( numConnected > 0 ) {
        numConnected = 0;
        
        for( int i=0; i<numClients; i++ ) {
            if( connections[i].sock == NULL ) {
                continue;
                }
            numConnected ++;
        
            if( connections[i].id == -1 ) {
                // still waiting for first PU

                if( connections[i].disconnected ) {
                    printf( "Client %d lost connection\n", i );

                    delete connections[i].sock;
                    connections[i].sock = NULL;
                    continue;
                    }

                char *message = getNextMessage( &( connections[i] ) );

                if( message != NULL ) {
                    //printf( "Client %d got message:\n%s\n\n", i, message );
                    
                    if( strstr( message, "MC" ) == message ) {
                        //printf( "Client %d got first map chunk\n", i );

                        int sizeX, sizeY, x, y, binarySize, compSize;
                        sscanf( message, "MC\n%d %d %d %d\n%d %d\n", 
                                &sizeX, &sizeY,
                                &x, &y, &binarySize, &compSize );
                    
                        connections[i].skipCompressedData = compSize;
                        }
                    else if( strstr( message, "PU" ) == message ) {
                        // PU message!
                    

                        // last line describes us

                        int numLines;
                        char **lines = split( message, "\n", &numLines );
                    
                        if( numLines > 2 ) {
                        
                            // first line is about us
                            parsePlayerUpdateMessage( &( connections[i] ),
                                                      lines[ numLines - 2 ] );
                            }
                    
                        printf( "Client %d got first player update, "
                                "pid = %d, pos = %d,%d\n", i, 
                                connections[i].id,
                                connections[i].x, 
                                connections[i].y );


                        
                        for( int p=0; p<numLines; p++ ) {
                            delete [] lines[p];
                            }
                        delete [] lines;
                        }
                    delete [] message;
                    }                
                }
            else {
                // player is live
                
                if( connections[i].dead ) {
                    printf( "Client %d died, closing connection\n", i );

                    delete connections[i].sock;
                    connections[i].sock = NULL;
                    continue;
                    }
                if( connections[i].disconnected ) {
                    printf( "Client %d lost connection\n", i );

                    delete connections[i].sock;
                    connections[i].sock = NULL;
                    continue;
                    }


                if( !connections[i].moving ) {
                
                    // make a move

                    connections[i].moving = true;
                
                    //printf( "Client %d starting move\n", i );
                    

                    int xDelt = 0;
                    int yDelt = 0;
                
                    while( xDelt == 0 && yDelt == 0 ) {
                        xDelt = randSource.getRandomBoundedInt( -1, 1 );
                        yDelt = randSource.getRandomBoundedInt( -1, 1 );
                        }
                
                
                    char *message = autoSprintf( "MOVE %d %d %d %d#",
                                                 connections[i].x,
                                                 connections[i].y,
                                                 xDelt, yDelt );
                
                    connections[i].sock->send( (unsigned char*)message, 
                                               strlen( message ), true, false );
                
                    delete [] message;
                    }

                char *message = getNextMessage( &( connections[i] ) );
        
                if( message != NULL ) {
                    if( strstr( message, "MC" ) == message ) {
                    
                        int sizeX, sizeY, x, y, binarySize, compSize;
                        sscanf( message, "MC\n%d %d %d %d\n%d %d\n", 
                                &sizeX, &sizeY,
                                &x, &y, &binarySize, &compSize );
                    
                        connections[i].skipCompressedData = compSize;
                        }
                    else if( strstr( message, "PU" ) == message ) {
                        // PU message!

                        int numLines;
                        char **lines = split( message, "\n", &numLines );
                    
                        for( int p=1; p<numLines-1; p++ ) {
                            parsePlayerUpdateMessage( &( connections[i] ),
                                                      lines[p] );
                            }

                        for( int p=0; p<numLines; p++ ) {
                            delete [] lines[p];
                            }
                        delete [] lines;
                        }
                    delete [] message;
                    }
            
                }
            }
        }
    
    printf( "No more clients connected\n" );
    
    for( int i=0; i<numClients; i++ ) {
        if( connections[i].sock != NULL ) {
            delete connections[i].sock;
            }
        }
    delete [] connections;
    
    return 1;
    }
