


// these calls are NOT thread safe for multiple calling threads


void initFileLoadThread();

void freeFileLoadThread();



typedef void * FileLoadHandle;


// inRelativeFilePath copied internally
FileLoadHandle startLoadingFile( const char *inRelativeFilePath );


char isFileLoaded( FileLoadHandle inHandle );



// only safe to call on files that return true for isFileLoaded
// gets contents of a loaded file
// return array destroyed by caller
// after this call, internal file resources are freed
unsigned char *getFileContents( FileLoadHandle inHandle,
                                int *outDataLength  );
