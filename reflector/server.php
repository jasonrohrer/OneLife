<?php


$tooFullFraction = .90;

$tooFullForTwinsFraction = .75;

$startSpreadingFraction = .50;

$stopSpreadingFraction = .10;


$updateServerURL = "http://onehouronelife.com/updateServer/server.php";


$reflectorLog = "/tmp/reflectorLog.txt";


function logMessage( $inMessage ) {
    global $reflectorLog;

    $d = date("M d, Y h:i:s A");
    
    file_put_contents( $reflectorLog, "$d $inMessage \n", FILE_APPEND );
    }


include( "requiredVersion.php" );

global $version;



function file_get_contents_safe( $inFileName ) {
    if( file_exists( $inFileName ) ) {
        return file_get_contents( $inFileName );
        }
    else {
        return FALSE;
        }
    }




$action = or_requestFilter( "action", "/[A-Z_]+/i", "" );
$email = or_requestFilter( "email", "/[A-Z0-9._%+-]+@[A-Z0-9.-]+/i", "" );

$twin_code = or_requestFilter( "twin_code", "/[A-F0-9]+/i", "" );
$twin_code = strtoupper( $twin_code );



//$ip = $_SERVER[ 'REMOTE_ADDR' ];

// seed the random number generator with their email adddress
// repeat calls under the same conditions send them to the same server
// this is better than using their IP address, because that can change
// Want to send people back to the same server tomorrow if load conditions
// are the same, even if their IP changes.
//mt_srand( crc32( $email ) );

// actually, don't make it sticky with their email address
// seed with their email address plus the current time
// so when they reconnect later (next life), they will be randomly
// sent to another server
// Note that this is not great for reconnects after crashes, but those
// should hopefully be rare
mt_srand( crc32( $email . time() ) );




if( $twin_code != "" ) {
    // all twins using same code go to same server
    // regardless of what email they use
    mt_srand( crc32( $twin_code ) );
    }



$reportOnly = false;



function getLastApocalypse() {
    $val = 0;

    $handle = fopen( "lastApocalypse.txt", "r" );

    if( $handle ) {
        $val = trim( fgets( $handle ) );

        fclose( $handle );
        }

    return $val;
    }




if( $action == "report" ) {
    $reportOnly = true;
    }
else if( $action == "check_apocalypse" ) {

    $val = getLastApocalypse();
    
    echo "$val\nOK";
    
    exit( 0 );
    }
else  if( $action == "trigger_apocalypse" ) {
    include( "sharedSecret.php" );

    global $sharedSecret;

    
    $id = or_requestFilter( "id", "/[0-9]+/", "-1" );

    if( $id == -1 || $id <= getLastApocalypse() ) {
        echo "DENIED";
        exit( 0 );
        }


    $id_hash = or_requestFilter( "id_hash", "/[A-F0-9]+/i", "" );

    $id_hash = strtoupper( $id_hash );

    $computedHashValue =
        strtoupper( or_hmac_sha1( $sharedSecret, $id ) );

    if( $id_hash != $computedHashValue ) {
        echo "DENIED";
        exit( 0 );
        }

    $name = or_requestFilter( "name", "/[A-Z ]+/i", "UNKNOWN" );

    $name = str_replace( " ", "_", $name );

    $time = time();
    
    file_put_contents( "lastApocalypse.txt", "$id" );

    $s = "s";
    if( $id == 1 ) {
        $s = "";
        }
    file_put_contents( "apocalypseStats.php", "$id apocalypse$s" );
    
    file_put_contents( "apocalypseLog.txt", "$id $time $name\n", FILE_APPEND );
    echo "OK";
    
    exit( 0 );
    }




$serverFound = false;

$handle = fopen( "remoteServerList.ini", "r" );

$totalPlayers = 0;
$totalCap = 0;


if( $handle ) {

    if( $reportOnly ) {
        $dateStamp = date("D M j G:i:s T Y");
        echo "$dateStamp<br><br>";
        echo "Remote servers:<br><br>";
        }
    
    $serverAddresses = array();
    $serverPorts = array();

    if( $reportOnly ) {
        
        while( ( !$serverFound && $line = fgets( $handle ) ) !== false ) {
            // process the line read.
            $parts = preg_split( "/\s+/", $line );
            
            if( count( $parts ) >= 3 ) {
                
                $address = $parts[1];
                $port = $parts[2];
                
                $serverFound = tryServer( $address, $port, $reportOnly, false );
                }
            }
        }
    else {
        
        $curNumServersFile = "/tmp/currentNumReflectorServers";

        $curNumServers = file_get_contents_safe( $curNumServersFile );
                
        if( $curNumServers === FALSE ) {
            $curNumServers = 1;
            file_put_contents( $curNumServersFile, $curNumServers );
            }

        $totalMaxCap = 0;
        $totalCurrentPop = 0;

        $maxCapPerServer = array();
        $currentPopPerServer = array();
        $offlineServerFlags = array();
        
        $totalNumServer = 0;
        $totalNumOnline = 0;
        
        while( ( $line = fgets( $handle ) ) !== false ) {
            // process the line read.
            $parts = preg_split( "/\s+/", $line );
        
            if( count( $parts ) >= 3 ) {
            
                $address = $parts[1];
                $port = $parts[2];

                $serverAddresses[] = $address;
                $serverPorts[] = $port;
            
                $maxFile = "/tmp/" .$address . "_" . $port . "_max";
                $currentFile = "/tmp/" .$address . "_" . $port . "_current";
                $offlineFile = "/tmp/" .$address . "_" . $port . "_offline";

                $max = file_get_contents_safe( $maxFile );
                $current = file_get_contents_safe( $currentFile );
                $offline = file_get_contents_safe( $offlineFile );

                if( $max === FALSE ||
                    $current === FALSE ||
                    $offline === FALSE ) {
                    // start with a sensible default,
                    // we know nothing about this server
                    $max = 100;
                    $current = 0;
                    $offline = 0;
                    }

                if( $offline == 1 &&
                    filemtime( $offlineFile ) < time() - 20 ) {
                    // offline, and hasn't been checked for 20 seconds

                    $onlineNow = tryServer( $address, $port, false,
                                            true, // test only, don't print
                                            true );

                    if( $onlineNow ) {
                        $offline = 0;
                        }
                    }
                
                
                $totalMaxCap += $max;
                $totalCurrentPop += $current;
                $maxCapPerServer[] = $max;
                $currentPopPerServer[] = $current;
                $offlineServerFlags[] = $offline;
                
                $totalNumServer ++;

                if( $offline == 0 ) {
                    $totalNumOnline ++;
                    }
                }
            }

        
        // sums for just our active server subset
        // but skip offline servers along the way
        $activeMaxCap = 0;
        $activeCurrentPop = 0;

        $i=0;
        $numServersSummed = 0;
        $currentActiveServerIndices = array();
        
        while( $i < $totalNumServer &&
               $numServersSummed < $curNumServers ) {

            if( $offlineServerFlags[$i] == 0 ) {
                $activeMaxCap += $maxCapPerServer[$i];
                $activeCurrentPop += $currentPopPerServer[$i];
                $numServersSummed++;
                $currentActiveServerIndices[] = $i;
                }
            $i++;
            }

        // never start spreading if we see ALL servers offline
        // we can't reach them, and we know nothing about them
        // our $activeMaxCap is 0 in that case, so our test is meaningless
        if( $numServersSummed > 0 &&
            $activeMaxCap > 0 &&
            $activeCurrentPop > 0 &&
            $curNumServers < $totalNumServer &&
            $activeMaxCap * $startSpreadingFraction <= $activeCurrentPop ) {
            
            logMessage( "$activeMaxCap * $startSpreadingFraction <= $activeCurrentPop, ".
                        "$curNumServers servers is not enough, adding another" );

            // we are over 50%
            // add another server for next time
            $curNumServers ++;

            file_put_contents( $curNumServersFile, $curNumServers );
            // don't adjust $activeMaxCap this time
            }
        // never STOP spreading if some of our servers are offline
        // we don't have accurate information about player population in
        // that case.  Leave spreading in place to avoid ping-ponging
        // during temporary outages
        // When in doubt, keep spreading.
        // Note that in the case of a long-term outtage, the server should
        // be removed from remoteServerList.ini manually so that it doesn't
        // interfere with stop-spreading detection.
        else if( $totalNumOnline == $totalNumServer &&
                 $curNumServers > 1 &&
                 $activeMaxCap * $stopSpreadingFraction >= $activeCurrentPop ) {

            logMessage( "$activeMaxCap * $stopSpreadingFraction >= $activeCurrentPop, ".
                        "$curNumServers servers is too many, removing one" );
            // below threshold
            // remove a server for next time
            $curNumServers --;

            file_put_contents( $curNumServersFile, $curNumServers );
            // don't adjust $activeMaxCap this time
            }


        // now pick a server using a probability distribution based on each
        // server's fraction of total max cap
        $serverFound = false;
        $tryCount = 0;
        
        while( ! $serverFound && $tryCount < 5 ) {
            $tryCount++;
            
            $pick = mt_rand() / mt_getrandmax();

            $i = 0;
            $totalWeight = 0;
            
            while( $i < $numServersSummed ) {
                $serverInd = $currentActiveServerIndices[$i];
                
                $totalWeight += $maxCapPerServer[$serverInd] / $activeMaxCap;

                $tooFull = false;
                if( $currentPopPerServer[$serverInd] /
                    $maxCapPerServer[$serverInd] >
                    $tooFullFraction ) {
                    $tooFull = true;
                    }
                else if( $twin_code != "" &&
                         $currentPopPerServer[$serverInd] /
                         $maxCapPerServer[$serverInd] >
                         $tooFullForTwinsFraction ) {
                    $tooFull = true;
                    }
                
                if( ! $tooFull &&
                    $totalWeight >= $pick ) {
                    break;
                    }
                $i++;
                }

            if( $i >= $numServersSummed ) {
                // something went wrong above, maybe precision errors
                $i = mt_rand( 0, $numServersSummed - 1 );
                }

            $serverInd = $currentActiveServerIndices[$i];
            
            $serverFound = tryServer( $serverAddresses[$serverInd],
                                      $serverPorts[$serverInd],
                                      $reportOnly, false, true );
            }

        if( !$serverFound ) {
            // yikes!
            // last ditch attempt to find some server, any server
            // for them to connect to
            $i = 0;
            while( $i < $totalNumServer &&
                   ! $serverFound ) {
                $serverFound = tryServer( $serverAddresses[$i],
                                          $serverPorts[$i],
                                          $reportOnly, false, true );
                $i++;
                }
            }
        }
    
    fclose( $handle );
    }

if( $reportOnly ) {
    echo "---------------------------<br><br>";
    echo "Total :::  $totalPlayers / $totalCap<br><br>";
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
function tryServer( $inAddress, $inPort, $inReportOnly,
                    $inTestOnly,
                    $inIgnoreSpreading = false ) {

    global $version, $startSpreadingFraction, $tooFullFraction,
        $tooFullForTwinsFraction, $twin_code,
        $stopSpreadingFraction, $updateServerURL;
    global $totalPlayers, $totalCap;
    
    $serverGood = false;

    $maxFile = "/tmp/" .$inAddress . "_" . $inPort . "_max";
    $currentFile = "/tmp/" .$inAddress . "_" . $inPort . "_current";
    $offlineFile = "/tmp/" .$inAddress . "_" . $inPort . "_offline";

    
    // suppress printed warnings from fsockopen
    // sometimes servers will be down, and we'll skip them.
    $fp = @fsockopen( $inAddress, $inPort, $errno, $errstr, 0.125 );
    if( !$fp ) {
        // error

        if( $inReportOnly ) {
            echo "|--> $inAddress : $inPort ::: OFFLINE<br><br>";
            }
        file_put_contents( $offlineFile, "1" );
        
        return false;
        }
    else {
        stream_set_timeout( $fp, 0, 125000 );
        
        $lineCount = 0;

        $accepting = false;
        $tooFull = false;
        $spreading = false;
        $stopSpreading = false;

        $current = -1;
        $max = -1;
        
        while( !feof( $fp ) && $lineCount < 2 ) {
            $line = fgets( $fp, 128 );

            $info = stream_get_meta_data( $fp );

            if( $info[ 'timed_out' ] ) {
                if( $inReportOnly ) {
                    echo "|--> $inAddress : $inPort ::: OFFLINE<br><br>";
                    }
                
                file_put_contents( $offlineFile, "1" );
                return false;
                }
            
            if( $lineCount == 0 && strstr( $line, "SN" ) ) {
                // accepting connections
                $accepting = true;
                }
            else if( $lineCount == 1 ) {


                sscanf( $line, "%d/%d", $current, $max );

                if( $inReportOnly ) {
                    echo "|--> $inAddress : $inPort ::: ".
                        "$current / $max<br><br>";
                    $totalPlayers += $current;
                    $totalCap += $max;
                    }
                
                if( $current / $max > $tooFullFraction ) {
                    $tooFull = true;
                    }
                else if( $twin_code != "" &&
                         $current / $max > $tooFullForTwinsFraction ) {
                    $tooFull = true;
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

        file_put_contents( $maxFile, "$max" );
        file_put_contents( $currentFile, "$current" );
        file_put_contents( $offlineFile, "0" );
        
        
        if( $inReportOnly ) {
            return false;
            }
        

        if( $accepting && ! $tooFull ) {

            $spreadingFile = "/tmp/" .$inAddress . "_" . $inPort . "_spreading";
            
            if( $spreading ) {

                if( ! file_exists( $spreadingFile ) ) {    
                    // remember that we've crossed the spreading
                    // threshold for this server
                    
                    $fileOut = fopen( $spreadingFile, "w" );
                    
                    if( $fileOut ) {                        
                        fwrite( $fileOut, "1" );

                        fclose( $fileOut );
                        }
                    logMessage( "$current / $max >= $startSpreadingFraction for server ".
                                "$inAddress, starting to spread to next server." );
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

                    logMessage( "$current / $max <= $stopSpreadingFraction for server ".
                                "$inAddress, stopping spreading." );
                    }
                
                $spreading = false;
                }


            if( $spreading && ! $inIgnoreSpreading ) {
                // flip coin and only use this server half the time

                if( mt_rand( 0, 1 ) == 0 ) {
                    return false;
                    }
                if( $twin_code == "" &&
                    $current / $max > $startSpreadingFraction ) {
                    // if not twins, send additional people to
                    // other servers until this one dies down below
                    // threshold

                    // so, what happens is this:
                    // once we pass threshold, all non-twins are sent to
                    // other servers
                    // When we die down below threshold, 50% of players
                    // are sent to other servers, until we get back
                    // below the stopSpreading fraction, and then we stop.
                    return false;
                    }  
                }

            // got here, return this server

            // we successfully sent another player there
            // update our count for this server right away
            $current++;
            file_put_contents( $currentFile, "$current" );


            if( ! $inTestOnly ) {    
                echo "$inAddress\n";
                echo "$inPort\n";
                echo "$version\n";
                echo "$updateServerURL\n";
                echo "OK";
                }
            
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


function or_hmac_sha1( $inKey, $inData ) {
    return hash_hmac( "sha1", 
                      $inData, $inKey );
    } 


?>