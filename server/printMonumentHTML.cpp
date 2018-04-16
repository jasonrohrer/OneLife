#include <stdlib.h>


#include "minorGems/io/file/File.h"
#include "minorGems/util/stringUtils.h"
#include "minorGems/util/MinPriorityQueue.h"


void usage() {
    printf( "Usage:\n" );
    printf( "printMonumentHTML path_to_server_dir path_to_monument_web_dir "
            "\n\n" );
    
    printf( "NOTE:  server dir can contain multiple monumentLogs dirs\n" );
    printf( "       (monumentLogs, monumentLogs_server2, etc.)\n\nd" );

    
    printf( "Example:\n" );
    printf( "printMonumentHTML "
            "~/checkout/OneLife/server "
            "~/www/monuments\n\n" );
    
    exit( 1 );
    }



char *nameFromLine( char *inLine ) {

    char hasTime = false;
    
    if( strstr( inLine, "time_" ) != NULL ) {
        hasTime = true;
        }
    


    SimpleVector<char*> *tok = tokenizeString( inLine );
    
    SimpleVector<char*> nameTok;

    if( tok->size() < 5 ) {
        tok->deallocateStringElements();

        delete tok;

        return stringDuplicate( "Nameless" );
        }
    
    
    if( hasTime && tok->size() == 8 
        ||
        ! hasTime && tok->size() == 7 ) {
        
        nameTok.push_back( tok->getElementDirect( 5 ) );
        nameTok.push_back( tok->getElementDirect( 6 ) );
        }
    else {
        nameTok.push_back( tok->getElementDirect( 5 ) );
        }
    
    SimpleVector<char> formatedNameChar;
    
    for( int i=0; i<nameTok.size(); i++ ) {
        char *lower = stringToLowerCase( nameTok.getElementDirect( i ) );
        
        lower[0] = toupper( lower[0] );

        formatedNameChar.appendElementString( lower );
        if( i < nameTok.size() - 1 ) {
            formatedNameChar.appendElementString( " " );
            }
        delete [] lower;
        }
    

    tok->deallocateStringElements();
    delete tok;

    return formatedNameChar.getElementString();
    }



void processLogFile( File *inFile, FILE *outHTMLFile ) {

    char *name = inFile->getFileName();
    
    int x, y;
    
    double timeStamp;
    
    int numRead = sscanf( name, "%d_%d_done_%lf.txt", 
                          &x, &y, &timeStamp );

    delete [] name;


    char *cont = inFile->readFileContents();
    
    int numLines;
    
    char **lines = split( cont, "\n", &numLines );

    delete [] cont;
    
    int numLinesToUse = numLines - 1;
    
    
    char *monName = lines[numLinesToUse];

    if( strlen( monName ) < 2 ) {
        // can be blank line at end
        numLinesToUse --;
        monName = lines[numLinesToUse];
        }
    
    
    char *poundLoc = strstr( monName, "#" );
    
    if( poundLoc != NULL ) {
        poundLoc[0] = '\0';
        }
    
    


    fprintf( outHTMLFile, "<tr><td>\n" );
    fprintf( outHTMLFile, "\n" );

    fprintf( outHTMLFile, "<font size=5><u>%s</u></font><br>\n", monName );
    

    // assume same epoch
    time_t t = (time_t)timeStamp;
    struct tm *timeStruct = localtime( &t );

    char timeString[100];

    strftime( timeString, 99, "%A, %B %d, %T %Z", timeStruct );
    
    fprintf( outHTMLFile, "%s", timeString );
    
    fprintf( outHTMLFile, "<br>\n" );

    fprintf( outHTMLFile, "Location: (%d, %d)<br><br>\n", x, y );
    fprintf( outHTMLFile, "<b><u>Contributors:</u></b><br>\n" );

    char *prevName = stringDuplicate( "" );
    
    for( int i=0; i<numLines-1; i++ ) {
        char *contribName = nameFromLine( lines[i] );

        if( strcmp( prevName, contribName ) != 0 ) {
            fprintf( outHTMLFile, "%s<br>\n", contribName );
            delete [] prevName;
            prevName = stringDuplicate( contribName );
            }
        delete [] contribName;
        }
    delete [] prevName;
    

    fprintf( outHTMLFile, "<br>\n" );
    fprintf( outHTMLFile, "<br>\n" );
    fprintf( outHTMLFile, "<br>\n" );
    fprintf( outHTMLFile, "<br>\n" );

    fprintf( outHTMLFile, "</td><tr>\n" );
    fprintf( outHTMLFile, "\n" );


    for( int i=0; i<numLines; i++ ) {
        delete [] lines[i];
        }
    delete [] lines;
    
    }



SimpleVector<char*> htmlFileNames;


void processMonumentLogsFolder( File *inFolder, File *inHTMLFolder ) {
    int numFiles;
    
    File **logs = inFolder->getChildFiles( &numFiles );

    FILE *outHTMLFile = NULL;

    char someDoneFiles = false;

    for( int i=0; i<numFiles && !someDoneFiles; i++ ) {
        char *name = logs[i]->getFileName();
        
        if( strstr( name, "_done_" ) != NULL ) {
            someDoneFiles = true;
            }
        delete [] name;
        }
    

    if( someDoneFiles ) {
        
        char serverName[200];
        char *name = inFolder->getFileName();
        
        int numRead = sscanf( name, "monumentLogs_%199s",
                              serverName );

        delete [] name;
        
        
        char *htmlFileName = NULL;

        if( numRead == 1 ) {
            htmlFileName = autoSprintf( "%s.php", serverName );
            }
        else {
            htmlFileName = stringDuplicate( "main.php" );
            }
        
        File *htmlFile = inHTMLFolder->getChildFile( htmlFileName );
        

        char *path = htmlFile->getFullFileName();

        delete htmlFile;
        
        outHTMLFile = fopen( path, "w" );

        if( outHTMLFile != NULL ) {
            htmlFileNames.push_back( htmlFileName );
            
            fprintf( outHTMLFile, "<?php include( \"../header.php\" ); ?>\n" );
            fprintf( outHTMLFile, "<center><table><br><br>\n" );
            }
        else {
            delete [] htmlFileName;
            }
        
        delete [] path;
        }
    
    if( outHTMLFile == NULL ) {
        for( int i=0; i<numFiles; i++ ) {
            delete logs[i];
            }
        delete [] logs;
        return;
        }


    // sort by timestamp
    MinPriorityQueue<File*> q;
    
    
    for( int i=0; i<numFiles; i++ ) {
        char *name = logs[i]->getFileName();
        
        if( strstr( name, "_done_" ) != NULL ) {
            
            int x, y;
            double timeStamp;
            
            int numRead = sscanf( name, "%d_%d_done_%lf.txt",
                                  &x, &y, &timeStamp );
            
            if( numRead == 3 ) {
                q.insert( logs[i], timeStamp );
                }
            }
        delete [] name;
        }
    

    while( q.size() > 0 ) {
        File *f = q.removeMin();
        
        processLogFile( f, outHTMLFile );
        }
    
    
    for( int i=0; i<numFiles; i++ ) {
        delete logs[i];
        }
    delete [] logs;
    

    if( outHTMLFile != NULL ) {
        fprintf( outHTMLFile, "</table></center>\n" );
        fprintf( outHTMLFile, "<?php include( \"../footer.php\" ); ?>\n" );
        fclose( outHTMLFile );
        }
    }







int main( int inNumArgs, char **inArgs ) {

    if( inNumArgs != 3 ) {
        usage();
        }

    char *path = inArgs[1];
    char *htmlPath = inArgs[2];

    if( path[strlen(path) - 1] == '/' ) {
        path[strlen(path) - 1] = '\0';
        }

    if( htmlPath[strlen(htmlPath) - 1] == '/' ) {
        htmlPath[strlen(htmlPath) - 1] = '\0';
        }
    
    File mainDir( NULL, path );
    File htmlDir( NULL, htmlPath );


    if( mainDir.exists() && mainDir.isDirectory() &&
        htmlDir.exists() && htmlDir.isDirectory() ) {

        int numChildFiles;
        File **childFiles = mainDir.getChildFiles( &numChildFiles );
        
        for( int i=0; i<numChildFiles; i++ ) {
        
            if( childFiles[i]->isDirectory() ) {
                
                char *name = childFiles[i]->getFileName();
                
                if( strstr( name, "monumentLogs" ) == name ) {
                    // file name starts with monumentLogs
                    processMonumentLogsFolder( childFiles[i],
                                               &htmlDir );
                    }
                delete [] name;
                }
            
            delete childFiles[i];
            }
        delete [] childFiles;


        File *indexFile = htmlDir.getChildFile( "index.php" );
        
        char *indexFilePath = indexFile->getFullFileName();
        
        delete indexFile;
        
        FILE *indexF = fopen( indexFilePath, "w" );

        delete [] indexFilePath;
        
        fprintf( indexF, "<?php include( \"../header.php\" ); ?>\n" );
        fprintf( indexF, "<center><table><tr><td><br>\n" );
        fprintf( indexF, "<font size=5>Monument Lists:</font><br><br>\n" );

        for( int i=0; i<htmlFileNames.size(); i++ ) {
            char *htmlName = htmlFileNames.getElementDirect( i );
            
            char *humanName = stringDuplicate( htmlName );
            
            char *phpLoc = strstr( humanName, ".php" );
            if( phpLoc != NULL ) {
                phpLoc[0] = '\0';
                }
            
            fprintf( indexF, "<a href=%s>%s</a><br><br>\n",
                     htmlName, humanName );
            

            delete [] humanName;
            delete [] htmlName;
            }

        fprintf( indexF, "<br><br><br></td></tr></table></center>\n" );
        fprintf( indexF, "<?php include( \"../footer.php\" ); ?>\n" );
        
        fclose( indexF );
        }
    else {
        usage();
        }


    return 0;
    }

