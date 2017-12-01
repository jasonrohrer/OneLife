#include <stdlib.h>


#include "minorGems/io/file/File.h"
#include "minorGems/util/stringUtils.h"


void usage() {
    printf( "Usage:\n" );
    printf( "printFoodLogStatsHTML path_to_server_dir path_to_objects_dir "
            "outHTMLFile\n\n" );
    
    printf( "NOTE:  server dir can contain multiple foodLog dirs\n" );
    printf( "       (foodLog, foodLog_server2, etc.)\n\nd" );

    
    printf( "Example:\n" );
    printf( "printFoodLogStatsHTML "
            "~/checkout/OneFood/server "
            "~/checkout/OneFood/server/objects out.html\n\n" );
    
    exit( 1 );
    }


typedef struct FoodRec {
        int id;
        int count;
        int value;
    } FoodRec;

    
SimpleVector<FoodRec> monthRecords;
SimpleVector<FoodRec> weekRecords;
SimpleVector<FoodRec> todayRecords;
SimpleVector<FoodRec> yesterdayRecords;
SimpleVector<FoodRec> hourRecords;


// to sort with largest value at the top
int compareFoodRec( const void *inA, const void *inB ) {
    FoodRec *a = (FoodRec*)inA;
    FoodRec *b = (FoodRec*)inB;
    
    if( a->value > b->value ) {
        return -1;
        }
    if( a->value < b->value ) {
        return 1;
        }
    return 0;
    }


void sortRecList( SimpleVector<FoodRec> *inRecList ) {
    int numRec = inRecList->size();

    if( numRec == 0 ) {
        return;
        }
    
    FoodRec *recArray = inRecList->getElementArray();
    
    inRecList->deleteAll();
    
    qsort( recArray, numRec, sizeof(FoodRec), compareFoodRec );
    
    inRecList->appendArray( recArray, numRec );
    
    delete [] recArray;
    }



FoodRec *findExistingRect( SimpleVector<FoodRec> *inRecList, int inID ) {
    for( int i=0; i<inRecList->size(); i++ ) {
        FoodRec *r = inRecList->getElement( i );
        
        if( r->id == inID ) {
            return r;
            }
        }
    return NULL;
    }



void addCountAndValue( SimpleVector<FoodRec> *inRecList, int inID,
                       int inCount, int inValue ) {

    FoodRec *r = findExistingRect( inRecList, inID );
    
    if( r != NULL ) {
        r->count += inCount;
        r->value += inValue;
        }
    else {
        FoodRec rNew = { inID, inCount, inValue };
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

    sscanf( name, "%d_%d%99[^_]_%d", &fileYear, &fileMonth, monthName, &fileDay );
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
            


            int id, count, value;
            double aveAge;
            int aveMapX, aveMapY;
            
            int numScanned = 
                fscanf( f, "id=%d count=%d value=%d av_age=%lf "
                        "av_mapX=%d av_mapY=%d\n",
                        &id, &count, &value, &aveAge, &aveMapX, &aveMapY );
            
            if( numScanned == 6 ) {

                addCountAndValue( &monthRecords, id, count, value );

                if( isThisWeek ) {
                    
                    addCountAndValue( &weekRecords, id, count, value );
                    
                    if( isToday ) {
                        addCountAndValue( &todayRecords, id, count, value );
                        if( isThisHour ) {
                            addCountAndValue( &hourRecords, id, count, value );
                            }
                        }
                    else if( isYesterday ) {
                        addCountAndValue( &yesterdayRecords, id, count, value );
                        }
                    }

                scannedLine = true;
                }
            }
        

        fclose( f );
        }
    delete [] path;
    }


void printTable( const char *inName, File *inObjectDir, FILE *inFile,
                 SimpleVector<FoodRec> *inRecList ) {
    
    fprintf( inFile, "<center>\n<b>%s</b>\n", inName );
    
    fprintf( inFile, "<table border=1 cellpadding=0><tr><td>\n" );
    fprintf( inFile, "<table border=0 cellpadding=10>\n" );
    
    if( inRecList->size() == 0 ) {
        fprintf( inFile, "<tr><td>(no data)</td></tr>\n" );
        }
    
    for( int i=0; i<inRecList->size(); i++ ) {
        
        FoodRec *r = inRecList->getElement( i );
        
        char *objFileName = autoSprintf( "%d.txt", r->id );
        
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
                
                fprintf( inFile, "<tr><td>%s</td><td>%d</td>"
                         "<td>%d</td></tr>\n",
                         objName, r->count, r->value );

                fclose( objFILE );
                }
            }
        delete objFile;
        }
    
    fprintf( inFile, "</table></table>\n</center><br><br><br><br>\n" );
    }



void processFoodLogFolder( File *inFolder ) {
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
                
                if( strstr( name, "foodLog" ) == name ) {
                    // file name starts with foodLog
                    processFoodLogFolder( childFiles[i] );
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
