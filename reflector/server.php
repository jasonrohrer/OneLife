<?php


$tooFullFraction = .95;

$startSpreadingFraction = .50;

$stopSpreadingFraction = .10;


$updateServerURL = "http://onehouronelife.com/updateServer/server.php";


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



$action = or_requestFilter( "action", "/[A-Z_]+/i" );


$reportOnly = false;

if( $action == "report" ) {
    $reportOnly = true;
    }



$serverFound = false;



if( $mainServerAddress != "" && $mainServerPort != -1 ) {

    if( $reportOnly ) {
        echo "Main server:<br>";
        }
    
    // connect to main server first and see if it has room
    
    $serverFound = tryServer( $mainServerAddress, $mainServerPort,
                              $reportOnly );
    }


if( !$serverFound ) {

    $handle = fopen( "remoteServerList.ini", "r" );
    if( $handle ) {

        if( $reportOnly ) {
            echo "<br><br>Remote servers:<br>";
            }
        
        while( ( !$serverFound && $line = fgets( $handle ) ) !== false ) {
            // process the line read.
            $parts = preg_split( "/\s+/", $line );

            if( count( $parts ) >= 3 ) {
                
                $address = $parts[1];
                $port = $parts[2];

                $serverFound = tryServer( $address, $port, $reportOnly );
                }
            }
        
        fclose( $handle );
        }
    }


if( !$serverFound && !$reportOnly ) {

    echo "NONE_FOUND\n";
    echo "0\n";
    echo "0\n";
    echo "0\n";
    echo "OK";
    }


// tries a server, and if it has room, tells client about it, and returns true
// returns false if server full
//
// $inReportOnly set to true means we print a report line for this server
//             and return false
function tryServer( $inAddress, $inPort, $inReportOnly ) {

    global $version, $startSpreadingFraction, $tooFullFraction,
        $stopSpreadingFraction, $updateServerURL;
    
    $serverGood = false;


    // suppress printed warnings from fsockopen
    // sometimes servers will be down, and we'll skip them.
    $fp = @fsockopen( $inAddress, $inPort, $errno, $errstr, 3 );
    if( !$fp ) {
        // error

        if( $inReportOnly ) {
            echo "|--> $inAddress : $inPort ::: OFFLINE<br>";
            }
        
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

                if( $inReportOnly ) {
                    echo "|--> $inAddress : $inPort ::: ".
                        "$current / $max<br><br>";
                    }
                
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

        if( $inReportOnly ) {
            return false;
            }
        

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

    

/**
 * Filters a $_REQUEST variable using a regex match.
 *
 * Returns "" (or specified default value) if there is no match.
 */
function or_requestFilter( $inRequestVariable, $inRegex, $inDefault = "" ) {
    if( ! isset( $_REQUEST[ $inRequestVariable ] ) ) {
        return $inDefault;
        }
    
    $numMatches = preg_match( $inRegex,
                              $_REQUEST[ $inRequestVariable ], $matches );

    if( $numMatches != 1 ) {
        return $inDefault;
        }
        
    return $matches[0];
    }



?>