#include "ahapGate.h"

#include "minorGems/util/SimpleVector.h"


typedef struct GrantActionRecord {
        char *email;

        WebRequest *request;
    } GrantActionRecord;



static SimpleVector<GrantActionRecord> records;