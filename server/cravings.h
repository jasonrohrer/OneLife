

typedef struct Craving {
        int foodID;
        int uniqueID;
    } Craving;


extern Craving noCraving;



Craving getCravedFood( int inLineageEveID, int inPlayerGenerationNumber,
                       Craving inLastCraved = noCraving );


// call periodically to free memory
// deletes records that contain uniqueID < inLowestUniqueID
void purgeStaleCravings( int inLowestUniqueID );
