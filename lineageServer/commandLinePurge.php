<?php

// edit settings.php to change server' settings
include( "settings.php" );

global $databaseServer,
    $databaseUsername, $databasePassword, $databaseName;

    
    
$ls_mysqlLink =
    mysqli_connect( $databaseServer, $databaseUsername, $databasePassword );
    
mysqli_select_db( $ls_mysqlLink, $databaseName );

$numDeleted = 1;

$totalDeleted = 0;

$startTime = time();


function getElapsedTimeString( $startTime ) {
    $elapsed = time() - $startTime;

    $hours = floor($elapsed / 3600);
    $minutes = floor(($elapsed % 3600) / 60);
    $seconds = $elapsed % 60;

    return "$hours:$minutes:$seconds";
    }



while( $numDeleted > 0 ) {
    
    $query = 
        "DELETE FROM lineageServer_lives WHERE 1 AND eve_life_id != 11430302 AND id != 11430302 AND eve_life_id != 8921255 AND id != 8921255 AND eve_life_id != 5919776 AND id != 5919776 AND eve_life_id != 11202621 AND id != 11202621 AND eve_life_id != 9050433 AND id != 9050433 AND eve_life_id != 5931117 AND id != 5931117 AND eve_life_id != 10676508 AND id != 10676508 AND eve_life_id != 10596873 AND id != 10596873 AND eve_life_id != 9195192 AND id != 9195192 AND eve_life_id != 5919680 AND id != 5919680 AND death_time < DATE_SUB( NOW(), INTERVAL 1 YEAR ) limit 1000;";


    $result = mysqli_query( $ls_mysqlLink, $query );

    if( $result == true ) {        
        $result = mysqli_query( $ls_mysqlLink, "SELECT ROW_COUNT();" );

        $numDeleted = ls_mysqli_result( $result, 0 );

        $totalDeleted += $numDeleted;

        $timeString = getElapsedTimeString( $startTime );
        
        echo "\nDeleted $numDeleted more, total deleted = $totalDeleted, ".
            "running for $timeString";
        }
    else {
        $numDeleted = 0;
        }
    }


$timeString = getElapsedTimeString( $startTime );

echo "\n\nDone, deleted $totalDeleted total, running for $timeString\n\n";


function ls_mysqli_result( $result, $number, $field=0 ) {
    mysqli_data_seek( $result, $number );
    $row = mysqli_fetch_array( $result );
    return $row[ $field ];
    }

?>