<?php

include( "settings.php" );

global $header, $footer;


eval( $header );

global $challengePrefix;


$challenge = ug_requestFilter( "challenge",
                               "/$challengePrefix"."[0-9]+/i",
                               "$challengePrefix" . "0" );

$response = ug_requestFilter( "response", "/[A-F0-9]+/i", "ABCDEF" );


$nextNumber = trim( file_get_contents( $serialNumberFilePath ) );

$challengeNumber = str_replace( $challengePrefix, "", $challenge );

if( $challengeNumber < $nextNumber ) {

    echo "Challenge value $challenge is stale.";
    
    eval( $footer );
    die();
    }


global $contentLeaderEmailFilePath;

$contentLeaderEmail = trim( file_get_contents( $contentLeaderEmailFilePath ) );

global $ticketServerURL;

$checkURL = "$ticketServerURL?action=check_ticket_hash".
    "&email=$contentLeaderEmail".
    "&hash_value=$response".
    "&string_to_hash=$challenge";

$result = file_get_contents( $checkURL );

if( preg_match( "/INVALID/", $result ) ||
    ! preg_match( "/VALID/", $result ) ) {
    
    echo "Hash check failed, url = $checkURL";
    
    eval( $footer );
    die();
    }

$newNum = $challengeNumber + 1;

if( ! file_put_contents( $serialNumberFilePath, $newNum ) ) {
    echo "Failed to update serial number after hash check.";
    
    eval( $footer );
    die();
    }

    

$action = ug_requestFilter( "action", "/[A-Z_]+/i" );


if( $action == "content_update" ) {
    global $updateScriptPath, $updateScriptLogPath;

    // pass in two arguments to indicate automation
    shell_exec( 'nohup $updateScriptPath a a > $updateScriptLogPath 2>&1 &' );

    global $entryPointURL;
    
    echo "Update started.<br><br>";

    echo "Go here to view log:<br>";

    echo "<a href='$entryPointURL'>$entryPointURL</a>";
    }
else if( $action == "view_log" ) {
    global $updateScriptLogPath;

    $logContents = file_get_contents( $updateScriptLogPath );

    echo "Log contents:<br><pre>$logContents</pre>";
    }


eval( $footer );




/**
 * Filters a $_REQUEST variable using a regex match.
 *
 * Returns "" (or specified default value) if there is no EXACT match.
 * Entire variable value must match regex, with no extra characters before
 * or after part matched by regex.
 */
function ug_requestFilter( $inRequestVariable, $inRegex, $inDefault = "" ) {
    if( ! isset( $_REQUEST[ $inRequestVariable ] ) ) {
        return $inDefault;
        }

    return ug_filter( $_REQUEST[ $inRequestVariable ], $inRegex, $inDefault );
    }


/**
 * Filters a value  using a regex match.
 *
 * Returns "" (or specified default value) if there is no EXACT match.
 * Entire value must match regex, with no extra characters before
 * or after part matched by regex.
 */
function ug_filter( $inValue, $inRegex, $inDefault = "" ) {
    
    $numMatches = preg_match( $inRegex,
                              $inValue, $matches );

    if( $numMatches != 1 ) {
        return $inDefault;
        }

    if( $matches[0] == $inValue ) {
        return $matches[0];
        }
    else {
        return $inDefault;
        }
    }


?>