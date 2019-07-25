<?php


include( "settings.php" );


$action = as_requestFilter( "action", "/[A-Z_]+/i", "" );



if( $action == "get_sequence_number" ) {
    $n = getSequenceNumber();
    echo "$n";
    }
else if( $action == "report_arc_end" ) {
    reportArcEnd();
    }


function reportArcEnd() {
    $server_name = as_requestFilter( "server_name", "/[A-Z0-9._-]+/i", "" );


    global $sharedSecret, $mainServerName;

    if( $server_name != $mainServerName ) {
        // ignore
        echo "OK";
        return;
        }

    
    $sequence_number = ar_requestFilter( "sequence_number", "/[0-9]+/", "0" );
    
    $hash = strtoupper( ar_requestFilter( "hash", "/[A-F0-9]+/i", "" ) );

    
    $n = getSequenceNumber();

    if( $sequence_number < $n ) {
        echo "stale sequence number";
        return;
        }

    $computedHashValue =
        strtoupper( as_hmac_sha1( $sharedSecret, $sequence_number ) );

    if( $hash != $computedHashValue ) {
        echo "DENIED";
        return;
        }

    global $sequenceNumberFile;

    $sequence_number += 1;
    file_put_contents( $sequenceNumberFile, "$sequence_number" );

    
    // authorized

    $seconds_per_year = ar_requestFilter( "seconds_per_year", "/[0-9]+/", "0" );

    if( $seconds_per_year > 0 ) {
        global $secondsPerYearFile;
        file_put_contents( $secondsPerYearFile, "$seconds_per_year" );
        }

    global $lastArcStartTimeFile;
    global $lastArcEndTimeFile;

    // last end time is now previous start time
    $oldEndTime = file_get_contents_safe( $lastArcEndTimeFile );

    $thisTime = time();

    
    if( $oldEndTime === FALSE ) {
        // repeat this time in there
        file_put_contents( $lastArcStartTimeFile, "$thisTime" );
        }
    else {
        file_put_contents( $lastArcStartTimeFile, "$oldEndTime" );
        }
    
    file_put_contents( $lastArcEndTimeFile, "$thisTime" );
    }




    
function getSequenceNumber() {
    global $sequenceNumberFile;
    
    $n = file_get_contents_safe( $sequenceNumberFile );
    
    if( $n === FALSE ) {
        $n = 1;
        file_put_contents( $sequenceNumberFile, "$n" );
        }
    return $n;
    }

    



/**
 * Filters a $_REQUEST variable using a regex match.
 *
 * Returns "" (or specified default value) if there is no match.
 */
function as_requestFilter( $inRequestVariable, $inRegex, $inDefault = "" ) {
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


function as_hmac_sha1( $inKey, $inData ) {
    return hash_hmac( "sha1", 
                      $inData, $inKey );
    } 



function file_get_contents_safe( $inFileName ) {
    if( file_exists( $inFileName ) ) {
        return file_get_contents( $inFileName );
        }
    else {
        return FALSE;
        }
    }


?>