


// starts a new export bundle
// clears/discards any current bundle without saving it
void initExportBundle();



// adds an object to the currrent export bundle
void addExportObject( int inObjectID );


// finalizes and saves the bundle as an .oxp or .oxz file in the exports folder
// depending on settings/compressExports.ini
void finalizeExportBundle( char *inExportName );



// FIXME:
// old function
void exportObject( int inObjectID );
