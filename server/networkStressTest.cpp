#include <stdio.h>

#include "minorGems/network/Socket.h"
#include "minorGems/network/SocketServer.h"
#include "minorGems/system/Thread.h"

#define buffSize 4096

int main() {
    printf( "Listening on port 9000\n" );
    
    SocketServer server( 9000, 10 );
    
    Socket *sock = server.acceptConnection( -1 );
    
    if( sock != NULL ) {
        
        printf( "Got connection\n" );
        

        char fail = false;
        int totalSent = 0;
        int sleepCount = 0;
        
        unsigned char buffer[buffSize];
        
        while( ! fail ) {
            
            const char *message = "This is a test message";
            int len = strlen( message );
            int numSent = 
                sock->send( buffer, 
                            buffSize, 
                            false, false );
            
            if( numSent != buffSize ) {
                fail = true;
                }
            else {
                totalSent += numSent;
                }
            
            Thread::staticSleep( 1 );
            }
        printf( "Total sent before fail:  %d\n", totalSent );
        
        delete sock;
        }
    
    }
