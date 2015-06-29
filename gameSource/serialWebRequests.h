

// provides serial versions of the web request calls in
// minorGems/game/game.h

// This ensures that web requests are processed in order, with
// early requests completing up to full server response before later
// requests are sent to the server.

// Prevents unexpected request inerleavings (important in this case because
// most requests have non-repetable sequence numbers in them 
// (to thwart replay attacks), and the server only tracks the last sequence
// number used.  If requests arrive at the server out-of-order, then
// the server rejects some of them as stale.



int startWebRequestSerial( const char *inMethod, const char *inURL,
                           const char *inBody );

int stepWebRequestSerial( int inHandle );


char *getWebResultSerial( int inHandle );

unsigned char *getWebResultSerial( int inHandle, int *outSize );



// 0 if running as normal
// 1 if seems slow (half of retry time passed)
// 2+ if retrying (where getWebResultSerial() - 1 is the retry count so far).
int getWebRequestRetryStatus( int inHandle );



// does not wait for request to finish before ending it
void clearWebRequestSerial( int inHandle );



// true if server shut down and client should stop immediately
char getServerShutdown();


// true if player permanently dead (out of fresh starts) and 
// client should stop immediately
char getPermadead();
