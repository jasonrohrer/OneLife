
#include <stdio.h>
#include <signal.h>


#include "minorGems/util/stringUtils.h"
#include "minorGems/util/SettingsManager.h"
#include "minorGems/util/SimpleVector.h"
#include "minorGems/network/SocketServer.h"



typedef struct LiveObject {
        int id;
        int x;
        int y;

        Socket *sock;
        
        char isNew;
        char firstMessageSent;
        char error;
        char deleteSent;
        
    } LiveObject;



SimpleVector<LiveObject> players;


int nextID = 0;


volatile char quit = false;

void intHandler( int inUnused ) {
    quit = true;
    }


int numConnections = 0;


int main() {

    printf( "Test server\n" );

    signal( SIGINT, intHandler );
    
    int port = 
        SettingsManager::getIntSetting( "port", 5077 );
    

    
    SocketServer server( port, 256 );

    printf( "Listening for connection on port %d\n", port );

    while( !quit ) {
        
        Socket *sock = server.acceptConnection( 0 );
        
        if( sock != NULL ) {
            printf( "Got connection\n" );
            numConnections ++;
            
            LiveObject newObject;
            newObject.id = nextID;
            nextID++;
            newObject.x = 0;
            newObject.y = 0;
            newObject.sock = sock;
            newObject.isNew = true;
            newObject.firstMessageSent = false;
            newObject.error = false;
            newObject.deleteSent = false;
            
            players.push_back( newObject );            
            
            printf( "New player connected as player %d\n", newObject.id );

            printf( "Listening for another connection on port %d\n", port );
            }

        int numLive = players.size();
        

        // listen for any messages from clients 
        char anyMoves = false;
        SimpleVector<char> newMoves;
        
        for( int i=0; i<numLive; i++ ) {
            LiveObject *nextPlayer = players.getElement( i );
            
            char buffer[100];
            
            int numRead = nextPlayer->sock->receive( (unsigned char*)buffer, 
                                                     99, 0 );
                        
            if( numRead > 0 ) {
                
                buffer[numRead] = '\0';

                int x, y;
                
                int numScanned = sscanf( buffer, "%d %d#", &x, &y );

                
                if( numScanned == 2 ) {
                    char *moveLine = autoSprintf( "%d %d %d\n", 
                                                  nextPlayer->id, x, y );
                    newMoves.appendElementString( moveLine );
                    
                    delete [] moveLine;
                   
                    nextPlayer->x = x;
                    nextPlayer->y = y;
 
                    anyMoves = true;
                    }
                else {
                    nextPlayer->error = true;
                    }
                }
            else if( numRead == -1 ) {
                nextPlayer->error = true;
                }
            else if( nextPlayer->isNew ) {
                // their first position is a move
                
                char *moveLine = autoSprintf( "%d %d %d\n", 
                                              nextPlayer->id, 
                                              nextPlayer->x, nextPlayer->y );
                newMoves.appendElementString( moveLine );
                
                delete [] moveLine;
                
                nextPlayer->isNew = false;

                anyMoves = true;
                }
            
            }
        
        
        // report any clients that have given an error as deleted
        for( int i=0; i<players.size(); i++ ) {
            LiveObject *nextPlayer = players.getElement(i);

            if( nextPlayer->error && ! nextPlayer->deleteSent ) {
                char *moveLine = autoSprintf( "%d X X\n", nextPlayer->id  );
                
                newMoves.appendElementString( moveLine );
                
                delete [] moveLine;
                
                nextPlayer->isNew = false;

                anyMoves = true;

                nextPlayer->deleteSent = true;
                }
            }
        

        char *moveMessage = NULL;
        int moveMessageLength = 0;
        if( anyMoves ) {
            newMoves.push_back( '#' );
            moveMessage = newMoves.getElementString();

            moveMessageLength = strlen( moveMessage );
            }
        

        
        // send updates to clients
        
        for( int i=0; i<numLive; i++ ) {
            
            LiveObject *nextPlayer = players.getElement(i);
            
            
            if( ! nextPlayer->firstMessageSent ) {
                
                // now send starting message
                SimpleVector<char> messageBuffer;
                int numPlayers = players.size();
            
                // must be last in message
                char *playersLine;
                
                for( int i=0; i<numPlayers; i++ ) {
                
                    LiveObject o = *( players.getElement( i ) );
                
                    char *messageLine = 
                        autoSprintf( "%d %d %d\n", o.id, o.x, o.y );
                    

                    if( o.id != nextPlayer->id ) {
                        messageBuffer.appendElementString( messageLine );
                        delete [] messageLine;
                        }
                    else {
                        // save until end
                        playersLine = messageLine;
                        }
                    }

                messageBuffer.appendElementString( playersLine );
                delete [] playersLine;
                
                messageBuffer.push_back( '#' );
            
                char *message = messageBuffer.getElementString();
                int messageLength = strlen( message );

                int numSent = 
                    nextPlayer->sock->send( (unsigned char*)message, 
                                            messageLength, 
                                            false, false );
                
                delete [] message;
                

                if( numSent == messageLength ) {
                    nextPlayer->firstMessageSent = true;
                    }
                else if( numSent == -1 ) {
                    nextPlayer->error = true;
                    }
                else {
                    // still not sent, try again later
                    }
                }
            else if( anyMoves ) {
                // this player has first message, ready for updates

                int numSent = 
                    nextPlayer->sock->send( (unsigned char*)moveMessage, 
                                            moveMessageLength, 
                                            false, false );

                if( numSent == -1 ) {
                    nextPlayer->error = true;
                    }
                }
            }

        if( moveMessage != NULL ) {
            delete [] moveMessage;
            }
        
        
        // handle closing any that have an error
        for( int i=0; i<players.size(); i++ ) {
            LiveObject *nextPlayer = players.getElement(i);

            if( nextPlayer->error && nextPlayer->deleteSent ) {
                printf( "Closing connection to player %d on error\n",
                        nextPlayer->id );
                
                delete nextPlayer->sock;
                players.deleteElement( i );
                i--;
                }
            }

        }
    

    printf( "Quitting...\n" );
    

    for( int i=0; i<players.size(); i++ ) {
        LiveObject *nextPlayer = players.getElement(i);
        delete nextPlayer->sock;
        }
    
    printf( "Done.\n" );


    return 0;
    }
