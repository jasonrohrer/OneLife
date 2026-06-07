#include "timeLogger.h"

#include "minorGems/system/Time.h"

#include <string.h>


typedef struct TimeLogRecord {
        double       t;
        const char  *tag;
    } TimeLogRecord;


#define MAX_NUM_TIME_LOG_RECORDS   100

static  TimeLogRecord  timeRecords[ MAX_NUM_TIME_LOG_RECORDS ];

static  int            numTimeRecords  =  0;



void clearTimeLog() {
    numTimeRecords = 0;
    }



void logTime( const char *inTag ) {

    int     lastRecord  =  -1;
    double  curTime     =  Time::getCurrentTime();
    
    if( numTimeRecords > 0 ) {
        lastRecord = numTimeRecords - 1;
        }

    if( lastRecord != -1
        &&
        strcmp( timeRecords[ lastRecord ].tag,
                inTag ) == 0 ) {

        // hit

        // record delta time

        timeRecords[ lastRecord ].t = curTime - timeRecords[ lastRecord ].t;
        }
    else if( numTimeRecords < MAX_NUM_TIME_LOG_RECORDS - 1 ) {
        // new record

        timeRecords[ numTimeRecords ].tag = inTag;
        timeRecords[ numTimeRecords ].t   = curTime;

        numTimeRecords++;
        }
    }



void printTimeLog( FILE *inOutputFile ) {

    int  i;

    for( i = 0;
         i < numTimeRecords;
         i ++ ) {

        fprintf( inOutputFile,
                 "   %s took:  %f  seconds\n",
                 timeRecords[ i ].tag,
                 timeRecords[ i ].t );
        }
    
    }

