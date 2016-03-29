#include "testFileLoadThread.h"


#include "minorGems/system/Thread.h"
#include "minorGems/system/MutexLock.h"
#include "minorGems/system/BinarySemaphore.h"

#include "minorGems/util/stringUtils.h"

#include "minorGems/io/file/File.h"

#include <stdio.h>


static int nextFileID = 0;


typedef struct FileQueueRecord{
        int id;
        
        char *filePath;
        
        // null until reading starts
        unsigned char *readData;
        int dataLength;


        char ready;
        char done;

        FileQueueRecord *nextRecord;
        
    } FileQueueRecord;


static FileQueueRecord *headFileRecord = NULL;
static FileQueueRecord *tailFileRecord = NULL;



static MutexLock queueLock;
static BinarySemaphore newFileAddedSemaphore;
static char threadStopSignal = false;

class FileLoadThread : public Thread {
        
        virtual void run() {
            printf( "Thread starting run()\n" );
            
            char stopSignal = false;            

            while( ! stopSignal ) {
                printf( "Thread executing next loop\n" );
            
                FileQueueRecord *workingRecord = NULL;
                queueLock.lock();
            
                workingRecord = headFileRecord;

                while( workingRecord != NULL && workingRecord->ready ) {
                    workingRecord = workingRecord->nextRecord;
                    }

                queueLock.unlock();

                if( workingRecord != NULL ) {
                
                    // read the file
                    File file( NULL, workingRecord->filePath );
                
                    delete [] workingRecord->filePath;
                    workingRecord->filePath = NULL;

                    workingRecord->readData = 
                        file.readFileContents( 
                            &( workingRecord->dataLength ) );

                
                    queueLock.lock();
                    workingRecord->ready = true;
                    queueLock.unlock();
                    }
                else {
                    newFileAddedSemaphore.wait();
                    }

                queueLock.lock();
                stopSignal = threadStopSignal;
                queueLock.unlock();
                }
            }
        
    };


    
static FileLoadThread loadThread;



void initFileLoadThread() {
    threadStopSignal = false;
    printf( "Calling start() on thread\n" );
    loadThread.start();
    }

    


void freeFileLoadThread() {
    queueLock.lock();
    threadStopSignal = true;
    queueLock.unlock();
    
    newFileAddedSemaphore.signal();
    
    loadThread.join();


    FileQueueRecord *nextRecord = headFileRecord;

    while( nextRecord != NULL ) {
        if( nextRecord->filePath != NULL ) {
            delete [] nextRecord->filePath;
            nextRecord->filePath = NULL;
            }
        if( nextRecord->readData != NULL ) {
            delete [] nextRecord->readData;
            nextRecord->readData = NULL;
            }
        
        FileQueueRecord *nextNextRecord = nextRecord->nextRecord;
        
        delete nextRecord;
        nextRecord = nextNextRecord;
        }
    }






FileLoadHandle startLoadingFile( const char *inRelativeFilePath ) {

    FileQueueRecord *newRecord = new FileQueueRecord;
    
    newRecord->id = nextFileID;
    nextFileID ++;
    
    newRecord->filePath = stringDuplicate( inRelativeFilePath );
    newRecord->readData = NULL;
    newRecord->ready = false;
    newRecord->done = false;
    newRecord->nextRecord = NULL;


    queueLock.lock();
    
    
    // remove done entries from front of queue
    // before adding new entry to tail
    while( headFileRecord != NULL && headFileRecord->done ) {
        
        // path and read data have already been NULL'd if it's done
        
        FileQueueRecord *nextRecord = headFileRecord->nextRecord;
        
        delete headFileRecord;
        headFileRecord = nextRecord;
        }
    
    if( headFileRecord == NULL ) {
        // entire queue emptied
        tailFileRecord = NULL;
        }

    
    if( headFileRecord == NULL ) {
        headFileRecord = newRecord;
        tailFileRecord = newRecord;
        }
    else {
        tailFileRecord->nextRecord = newRecord;
        tailFileRecord = newRecord;
        }
    queueLock.unlock();
    

    newFileAddedSemaphore.signal();


    return newRecord;
    }



char isFileLoaded( FileLoadHandle inHandle ) {

    FileQueueRecord *record = (FileQueueRecord*) inHandle;

    
    // need lock to check this, even though it is atomic
    // (because of cache coherency issues---a lock ensures that all
    // threads are looking at the same memory image)
    
    char ready;
    

    queueLock.lock();
    ready = record->ready;
    queueLock.unlock();


    return ready;
    }



unsigned char *getFileContents( FileLoadHandle inHandle,
                                int *outDataLength ) {
    
    FileQueueRecord *record = (FileQueueRecord*) inHandle;
    
    // only need lock around setting of done flag

    unsigned char *returnData = record->readData;
    *outDataLength = record->dataLength;
    
    record->readData = NULL;
    
    
    queueLock.lock();
    record->done = true;
    queueLock.unlock();
    

    
    // don't remove it from our queue because it could make a hole
    // in the middle of our list
    // Thus, we always need to remove blocks of done records
    // from the head of our list.
    // we prune queue in startLoadingFile above


    return returnData;
    }
