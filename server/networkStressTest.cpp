#include <stdio.h>

#include "minorGems/network/Socket.h"
#include "minorGems/network/SocketServer.h"
#include "minorGems/system/Thread.h"

#define buffSize 8192

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
        
        int totalSleep = 500;
        
        while( ! fail ) {
            
            int splitCount = 64;
            int len = buffSize / splitCount;
            
            int splitSleep = totalSleep / splitCount;
            
            for( int i=0; i<splitCount; i++ ) {
                
                int numSent = 
                    sock->send( &( buffer[ i * len ] ), 
                                len, 
                                false, false );
                
                if( numSent != len ) {
                    fail = true;
                    }
                else {
                    totalSent += numSent;
                    }
                Thread::staticSleep( splitSleep );
                }
            // sleep after every full buffer
            //Thread::staticSleep( 1000 );
            if( totalSent % 131072 == 0 ) {
                printf( "Sent %d\n", totalSent );
                }
            }
        printf( "Total sent before fail:  %d\n", totalSent );
        
        delete sock;
        }
    
    }
