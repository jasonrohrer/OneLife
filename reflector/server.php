<?php


$tooFullFraction = .95;

$startSpreadingFraction = .50;

$stopSpreadingFraction = .10;


$updateServerURL = "http://192.168.1.124/jcr13/updateServer/server.php";


include( "requiredVersion.php" );

global $version;


$mainServerAddress = "";
$mainServerPort = -1;


$handle = fopen( "mainServer.ini", "r" );
if( $handle ) {
    while( ( $line = fgets( $handle ) ) !== false ) {
        // process the line read.
        $parts = preg_split( "/\s+/", $line );

        if( count( $parts ) >= 3 ) {
            
            $mainServerAddress = $parts[1];
            $mainServerPort = $parts[2];
            }
        }

    fclose( $handle );
    } 


$serverFound = false;

if( $mainServerAddress != "" && $mainServerPort != -1 ) {

    // connect to main server first and see if it has room

    $serverFound = tryServer( $mainServerAddress, $mainServerPort );
    }


if( !$serverFound ) {

    $handle = fopen( "remoteServerList.ini", "r" );
    if( $handle ) {
        while( ( !$serverFound && $line = fgets( $handle ) ) !== false ) {
            // process the line read.
            $parts = preg_split( "/\s+/", $line );

            if( count( $parts ) >= 3 ) {
                
                $address = $parts[1];
                $port = $parts[2];

                $serverFound = tryServer( $address, $port );
                }
            }
        
        fclose( $handle );
        }
    }


if( !$serverFound ) {

    echo "NONE_FOUND\n";
    echo "0\n";
    echo "0\n";
    echo "0\n";
    echo "OK";
    }


// tries a server, and if it has room, tells client about it, and returns true
// returns false if server full
function tryServer( $inAddress, $inPort ) {
    global $version, $startSpreadingFraction, $tooFullFraction,
        $stopSpreadingFraction, $updateServerURL;
    
    $serverGood = false;


    // suppress printed warnings from fsockopen
    // sometimes servers will be down, and we'll skip them.
    $fp = @fsockopen( $inAddress, $inPort, $errno, $errstr, 3 );
    if( !$fp ) {
        // error
        return false;
        }
    else {

        $lineCount = 0;

        $accepting = false;
        $tooFull = false;
        $spreading = false;
        $stopSpreading = false;
        
        while( !feof( $fp ) && $lineCount < 2 ) {
            $line = fgets( $fp, 128 );

            if( $lineCount == 0 && strstr( $line, "SN" ) ) {
                // accepting connections
                $accepting = true;
                }
            else if( $lineCount == 1 ) {

                $current = -1;
                $max = -1;

                sscanf( $line, "%d/%d", $current, $max );
                
                if( $current / $max > $tooFullFraction ) {
                    $tooFull = $true;
                    }
                else if( $current / $max >= $startSpreadingFraction ) {
                    $spreading = true;
                    }
                else if( $current / $max <= $stopSpreadingFraction ) {
                    $stopSpreading = true;
                    }
                }
            
            $lineCount ++;
            }
        
        fclose( $fp );


        if( $accepting && ! $tooFull ) {

            $spreadingFile = "/tmp/" .$inAddress . "_spreading";
            
            if( $spreading ) {

                if( ! file_exists( $spreadingFile ) ) {    
                    // remember that we've crossed the spreading
                    // threshold for this server
                    
                    $fileOut = fopen( $spreadingFile, "w" );
                    
                    if( $fileOut ) {                        
                        fwrite( $fileOut, "1" );

                        fclose( $fileOut );
                        }
                    }
                }
            else if( file_exists( $spreadingFile ) ) {
                $spreading = true;
                }

            if( $stopSpreading ) {
                // we've dropped back down below the spreading fraction
                // stop sending people to servers further down
                // the list from this one
                if( file_exists( $spreadingFile )  ) {
                    unlink( $spreadingFile );
                    }
                
                $spreading = false;
                }


            if( $spreading ) {
                // flip coin and only use this server half the time

                if( mt_rand( 0, 1 ) == 0 ) {
                    return false;
                    }
                }

            // got here, return this server

            echo "$inAddress\n";
            echo "$inPort\n";
            echo "$version\n";
            echo "$updateServerURL\n";
            echo "OK";

            return true;
            
            }
        else {
            return false;
            }
        }
    }

    

?>