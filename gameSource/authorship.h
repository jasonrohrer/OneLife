
#include "minorGems/crypto/hashes/sha1.h"
#include "minorGems/util/SettingsManager.h"




// authorship hash uses only the first 10 characters of the HMAC
// We expect 1 collision in this space if we have 1.5 million unique
// contributors, according to the formula here:
// https://matt.might.net/articles/counting-hash-collisions/

// resulting string destoyed by caller
inline char *getAuthorHash() {

    // use a fixed salt here so that authorship hashes are consistent

    // this doesn't serve much purpose, other than to prevent our hashes
    // from being looked up in other tables of bare-email hashes
    
    // Someone could, of course, re-hash a database of emails using this
    // known public salt and then map our hashes to an email in that database.
    const char *salt = "ohol_authorhsip_salt";
    

    // look up email fresh each time here
    // editor code doesn't load email as a global variable

    // if no email set, use blank as default author
    char *email = SettingsManager::getStringSetting( "email", "" );


    char *hash = hmac_sha1( salt, email );

    delete [] email;

    // terminate at 10 characters
    hash[10] = '\0';
    
    char *result = stringDuplicate( hash );
    
    delete [] hash;

    return result;
    }