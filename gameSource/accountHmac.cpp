#include "accountHmac.h"


#include "minorGems/util/stringUtils.h"
#include "minorGems/crypto/hashes/sha1.h"



extern char *accountKey;
extern int serverSequenceNumber;
extern int accountHmacVersionNumber;


char *getPureAccountKey() {
    const char *codeToHash = "";

    if( accountKey != NULL ) {
        codeToHash = accountKey;
        }

    
    // strip out "-" separators
    int numParts;
    char **codeParts = split( codeToHash, "-", &numParts );
    
    char *pureCode =
        join( codeParts, numParts, "" );
    
    for( int i=0; i<numParts; i++ ) {
        delete [] codeParts[i];
        }
    delete [] codeParts;

    return pureCode;
    }


char *getAccountHmac() {

    char *pureCode = getPureAccountKey();    


    char *toHash = autoSprintf( "%d%d", 
                                serverSequenceNumber,
                                accountHmacVersionNumber );
    
    char *hash = hmac_sha1( pureCode, toHash );
    
    delete [] pureCode;
    delete [] toHash;

    char *result = autoSprintf( "&sequence_number=%d&account_hmac=%s",
                                serverSequenceNumber,
                                hash );
    delete [] hash;

    serverSequenceNumber++;
    
    return result;
    }



char *replaceAccountHmac( char *inString ) {
    const char *keyA = "&sequence_number=";
    const char *keyB = "&account_hmac=";
    
    if( inString == NULL ) {
        return NULL;
        }
    else if( strstr( inString, keyA ) != NULL &&
             strstr( inString, keyB ) != NULL ) {
        
        // present

        char *copy = stringDuplicate( inString );
        
        char *startPointerA = strstr( copy, keyA );
        char *startPointerB = strstr( copy, keyB );
 
        char *hmacStart = &( startPointerB[ strlen( keyB ) ] );
        
        
        int i = 0;
        while( hmacStart[i] != '\0' &&
               ( ( hmacStart[i] >= '0' && 
                   hmacStart[i] <= '9' )
                 || 
                 ( hmacStart[i] >= 'A' &&
                   hmacStart[i] <= 'F' ) ) ) {
            i++;
            }
        
        // truncate here, after end of hmac
        hmacStart[i] = '\0';
        
        
        // now startPointerA points to the full hash
        
        char *newHash = getAccountHmac();

        char found;
        
        char *newHashInPlace = replaceOnce( inString, startPointerA,
                                            newHash,
                                            &found );

        delete [] newHash;
        delete [] copy;
        delete [] inString;

        return newHashInPlace;
        }
    else {
        // no hash present
        char *result = stringDuplicate( inString );
        delete [] inString;
        return result;
        }
    }

        
