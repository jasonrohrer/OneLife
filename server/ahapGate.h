
// asynchronous interface for server to trigger grant action on remote ahapGate


void initAHAPGate();


void freeAHAPGate();


typedef struct AHAPGrantResult {
        char *email;
        char *steamKey;
        char *accountURL;
    } AHAPGrantResult;


// if a remote grant action just finished successfully, returns pointer
// to heap-allocated struct with result.
// All char* elements of struct, and struct itself, destroyed by caller.
//
// returns NULL if no grant action just finished
AHAPGrantResult *stepAHAPGate();


// queues a new remote grant action
// inEmail destroyed by caller
void triggerAHAPGrant( const char *inEmail );


