#include <stdlib.h>


#include "minorGems/io/file/File.h"
#include "minorGems/util/stringUtils.h"


void usage() {
    printf( "Usage:\n" );
    printf( "printFailureLogStatsHTML path_to_server_dir path_to_objects_dir "
            "outHTMLFile\n\n" );
    
    printf( "NOTE:  server dir can contain multiple failureLog dirs\n" );
    printf( "       (failureLog, failureLog_server2, etc.)\n\nd" );

    
    printf( "Example:\n" );
    printf( "printFailureLogStatsHTML "
            "~/checkout/OneFailure/server "
            "~/checkout/OneFailure/server/objects out.html\n\n" );
    
    exit( 1 );
    }


typedef struct FailureRec {
        int actorID;
        int targetID;
        int count;
    } FailureRec;

    
SimpleVector<FailureRec> monthRecords;
SimpleVector<FailureRec> weekRecords;
SimpleVector<FailureRec> todayRecords;
SimpleVector<FailureRec> yesterdayRecords;
SimpleVector<FailureRec> hourRecords;


// to sort with largest value at the top
int compareFailureRec( const void *inA, const void *inB ) {
    FailureRec *a = (FailureRec*)inA;
    FailureRec *b = (FailureRec*)inB;
    
    if( a->count > b->count ) {
        return -1;
        }
    if( a->count < b->count ) {
        return 1;
        }
    return 0;
    }


void sortRecList( SimpleVector<FailureRec> *inRecList ) {
    int numRec = inRecList->size();

    if( numRec == 0 ) {
        return;
        }
    
    FailureRec *recArray = inRecList->getElementArray();
    
    inRecList->deleteAll();
    
    qsort( recArray, numRec, sizeof(FailureRec), compareFailureRec );
    
    inRecList->appendArray( recArray, numRec );
    
    delete [] recArray;
    }



FailureRec *findExistingRect( SimpleVector<FailureRec> *inRecList, 
                              int inActorID, int inTargetID ) {
    for( int i=0; i<inRecList->size(); i++ ) {
        FailureRec *r = inRecList->getElement( i );
        
        if( r->actorID == inActorID && r->targetID == inTargetID ) {
            return r;
            }
        }
    return NULL;
    }



void addCount( SimpleVector<FailureRec> *inRecList, int inActorID,
               int inTargetID, int inCount ) {

    FailureRec *r = findExistingRect( inRecList, inActorID, inTargetID );
    
    if( r != NULL ) {
        r->count += inCount;
        }
    else {
        FailureRec rNew = { inActorID, inTargetID, inCount };
        inRecList->push_back( rNew );
        }
    }



void processLogFile( File *inFile ) {
    
    char *path = inFile->getFullFileName();

    char isThisWeek = false;
    char isToday = false;
    char isYesterday = false;
    char isThisHour = false;
    

    char *name = inFile->getFileName();
    
    int fileYear, fileMonth, fileDay;

    char monthName[100];

    sscanf( name, "%d_%d%99[^_]_%d", 
            &fileYear, &fileMonth, monthName, &fileDay );
    struct tm fileTimeStruct;

    time_t t = time( NULL );
    fileTimeStruct = *( localtime ( &t ) );

    fileTimeStruct.tm_year = fileYear - 1900;
    fileTimeStruct.tm_mon = fileMonth - 1;
    fileTimeStruct.tm_mday = fileDay;

    time_t fileT = mktime( &fileTimeStruct );
    

    int numDaysInFileMonth;  
    if( fileMonth == 4 || fileMonth == 6 || 
        fileMonth == 9 || fileMonth == 11 ) {
        numDaysInFileMonth = 30;  
        } 
    else if( fileMonth == 2 ) { 
        char isLeapYear = 
            ( fileYear % 4 == 0 && fileYear % 100 != 0 ) || 
            ( fileYear % 400 == 0 );  
        if( isLeapYear ) {
            numDaysInFileMonth = 29;
            }
        else {
            numDaysInFileMonth = 28;
            }
        }  
    else  {
        numDaysInFileMonth = 31;  
        }
    
    
    struct tm *timeStruct = localtime( &t );
    
    int currentYear = timeStruct->tm_year + 1900;
    int currentDay = timeStruct->tm_mday;
    int currentMonth = timeStruct->tm_mon + 1;
    int currentYearDay = timeStruct->tm_yday;
    int currentHour = timeStruct->tm_hour;

    if( currentYear == fileYear &&
        currentMonth == fileMonth &&
        currentDay == fileDay ) {
        
        isToday = true;
        }


    double secDiff = difftime( t, fileT );
    
    if( secDiff < 7 * 24 * 3600 ) {
        isThisWeek = true;
        }
    
    
    if( currentYearDay == 0 ) {
        // jan 1
        if( currentYear - 1 == fileYear &&
            fileMonth == 12 &&
            fileDay == 31 ) {
            
            isYesterday = true;
            }
        }
    else {
        
        // today is mid-month
        if( currentDay > 1 &&
            currentYear == fileYear &&
            currentDay - 1 == fileDay ) {
            isYesterday = true;
            }
        // today is first day of month
        else if( currentDay == 1 &&
                 currentYear == fileYear &&
                 currentMonth - 1 == fileMonth &&
                 fileDay == numDaysInFileMonth ) {
            isYesterday = true;
            }
        }
    

    delete [] name;
    

    
    FILE *f = fopen( path, "r" );
    
    if( f != NULL ) {
        char scannedLine = true;
        
        while( scannedLine ) {
            scannedLine = false;
            
            int hour = 0;
            int lastScannedHour = 0;
            
            // process empty hours until next line is not an hour line
            while( hour != -1 ) {
                hour = -1;
                fscanf( f, "hour=%d\n", &hour );
                
                if( hour != -1 ) {
                    lastScannedHour = hour;
                    }
                }
            
            if( lastScannedHour != -1 && 
                isToday &&
                ( lastScannedHour == currentHour ||
                  lastScannedHour == currentHour - 1 ) ) {
                // if server running, this hour's data not recorded yet
                isThisHour = true;
                }
            


            int actorID, targetID, count;
            
            int numScanned = 
                fscanf( f, "%d + %d  count=%d\n",
                        &actorID, &targetID, &count);
            
            if( numScanned == 3 ) {

                addCount( &monthRecords, actorID, targetID, count );

                if( isThisWeek ) {
                    
                    addCount( &weekRecords, actorID, targetID, count );
                    
                    if( isToday ) {
                        addCount( &todayRecords, actorID, targetID, count );
                        if( isThisHour ) {
                            addCount( &hourRecords, actorID, targetID, count );
                            }
                        }
                    else if( isYesterday ) {
                        addCount( &yesterdayRecords, actorID, targetID, count );
                        }
                    }

                scannedLine = true;
                }
            }
        

        fclose( f );
        }
    delete [] path;
    }



// destroyed by caller if not NULL
char *getObjectName( File *inObjectDir, int inObjectID ) {
    char *name = NULL;
    
    char *objFileName = autoSprintf( "%d.txt", inObjectID );
    
    File *objFile = inObjectDir->getChildFile( objFileName );
    delete [] objFileName;
    
    if( objFile->exists() ) {
        char *fullName = objFile->getFullFileName();
            
        FILE *objFILE = fopen( fullName, "r" );
        delete [] fullName;
        
        if( objFILE != NULL ) {
            int id;
            char objName[100];
            int numRead = fscanf( objFILE,
                                  "id=%d\n" 
                                  "%99[^\n]", &id, objName );
            
            name = stringDuplicate( objName );
            fclose( objFILE );
            }
        delete objFile;
        }

    return name;
    }



void printTable( const char *inName, File *inObjectDir, FILE *inFile,
                 SimpleVector<FailureRec> *inRecList ) {
    
    fprintf( inFile, "<center>\n<b>%s</b>\n", inName );
    
    fprintf( inFile, "<table border=1 cellpadding=0><tr><td>\n" );
    fprintf( inFile, "<table border=0 cellpadding=10>\n" );
    
    if( inRecList->size() == 0 ) {
        fprintf( inFile, "<tr><td>(no data)</td></tr>\n" );
        }
    
    int limit = inRecList->size();
    
    // only show top 100
    if( limit > 100 ) {
        limit = 100;
        }
    
    for( int i=0; i<limit; i++ ) {
        
        FailureRec *r = inRecList->getElement( i );

        char *actorName = getObjectName( inObjectDir, r->actorID );
        char *targetName = getObjectName( inObjectDir, r->targetID );
        
        if( actorName != NULL && targetName != NULL ) {
            
            fprintf( inFile, "<tr><td>[ %s ] + [ %s ]</td><td>%d</td></tr>\n",
                     actorName, targetName, r->count );
            }

        if( actorName != NULL ) {
            delete [] actorName;
            }
        if( targetName != NULL ) {
            delete [] targetName;
            }
        }
    
    fprintf( inFile, "</table></table>\n</center><br><br><br><br>\n" );
    }



void processFailureLogFolder( File *inFolder ) {
    int numFiles;
    
    File **logs = inFolder->getChildFilesSorted( &numFiles );
    
    // only process last 30 files, if there are more than 30
    int startI = 0;
    
    if( numFiles > 30 ) {
        startI = numFiles - 30;
        }

    for( int i=startI; i<numFiles; i++ ) {
        processLogFile( logs[i] );
        }
    
    for( int i=0; i<numFiles; i++ ) {
        delete logs[i];
        }
    
    delete [] logs;
    }



int main( int inNumArgs, char **inArgs ) {

    if( inNumArgs != 4 ) {
        usage();
        }
    
    char *path = inArgs[1];
    char *objPath = inArgs[2];
    
    char *outPath = inArgs[3];
    
    if( path[strlen(path) - 1] == '/' ) {
        path[strlen(path) - 1] = '\0';
        }

    if( objPath[strlen(objPath) - 1] == '/' ) {
        objPath[strlen(objPath) - 1] = '\0';
        }
    
    File mainDir( NULL, path );
    File objDir( NULL, objPath );
    
    if( mainDir.exists() && mainDir.isDirectory() &&
        objDir.exists() && objDir.isDirectory() ) {

        int numChildFiles;
        File **childFiles = mainDir.getChildFiles( &numChildFiles );
        
        for( int i=0; i<numChildFiles; i++ ) {
        
            if( childFiles[i]->isDirectory() ) {
                
                char *name = childFiles[i]->getFileName();
                
                if( strstr( name, "failureLog" ) == name ) {
                    // file name starts with failureLog
                    processFailureLogFolder( childFiles[i] );
                    }
                delete [] name;
                }
            
            delete childFiles[i];
            }
        delete [] childFiles;

        

        sortRecList( &monthRecords );
        sortRecList( &weekRecords );
        sortRecList( &todayRecords );
        sortRecList( &yesterdayRecords );
        sortRecList( &hourRecords );

        FILE *outFile = fopen( outPath, "w" );
        

        if( outFile != NULL ) {
            
            printTable( "Past Hour",
                        &objDir, outFile, &hourRecords );
            
            printTable( "Today (so far)",
                        &objDir, outFile, &todayRecords );
            
            printTable( "Yesterday",
                        &objDir, outFile, &yesterdayRecords );
            
            printTable( "Past week",
                        &objDir, outFile, &weekRecords );

            printTable( "Past month",
                        &objDir, outFile, &monthRecords );
        
            fclose( outFile );
            }
        
        }
    else {
        usage();
        }
    
    }
