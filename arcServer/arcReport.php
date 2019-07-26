<?php

include( "settings.php" );


function file_get_contents_safe( $inFileName ) {
    if( file_exists( $inFileName ) ) {
        return file_get_contents( $inFileName );
        }
    else {
        return FALSE;
        }
    }



global $secondsPerYearFile, $lastArcEndTimeFile, $lastArcStartTimeFile;


$lastArcStartTime = file_get_contents_safe( $lastArcStartTimeFile );

$lastArcEndTime = file_get_contents_safe( $lastArcEndTimeFile );

$secondsPerYear = file_get_contents_safe( $secondsPerYearFile );


if( $secondsPerYear === FALSE  ) {
    // default
    $secondsPerYear = 60;
    }

$thisTime = time();

if( $lastArcStartTime === FALSE ) {
    $lastArcStartTime = $thisTime;
    file_put_contents( $lastArcStartTimeFile, "$thisTime" );
    }

if( $lastArcEndTime === FALSE ) {
    $lastArcEndTime = $thisTime;
    file_put_contents( $lastArcEndTimeFile, "$thisTime" );
    }

$years = floor( ( $thisTime - $lastArcEndTime ) / $secondsPerYear );

$yearWord = "years";

if( $years == 1 ) {
    $yearWord = "year";
    }

echo "Current player arc has been going $years $yearWord";

if( $lastArcStartTime != $lastArcEndTime ) {
    $prevYears = floor( ( $lastArcEndTime - $lastArcStartTime ) /
                        $secondsPerYear );

    $yearWord = "years";

    if( $prevYears == 1 ) {
        $yearWord = "year";
        }

    echo "<br>Previous arc lasted $prevYears $yearWord";
    }

?>