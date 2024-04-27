<?php



global $ag_version;
$ag_version = "1";



// edit settings.php to change server' settings
include( "settings.php" );




// no end-user settings below this point





// no caching
//header('Expires: Mon, 26 Jul 1997 05:00:00 GMT');
header('Cache-Control: no-store, no-cache, must-revalidate');
header('Cache-Control: post-check=0, pre-check=0', FALSE);
header('Pragma: no-cache'); 



// enable verbose error reporting to detect uninitialized variables
error_reporting( E_ALL );


// for stack trace of errors
/*
set_error_handler(function($severity, $message, $file, $line) {
    if (error_reporting() & $severity) {
        throw new ErrorException($message, 0, $severity, $file, $line);
    }
});
*/




// page layout for web-based setup
$setup_header = "
<HTML>
<HEAD><TITLE>AHAP Gate Server Web-based setup</TITLE></HEAD>
<BODY BGCOLOR=#FFFFFF TEXT=#000000 LINK=#0000FF VLINK=#FF0000>

<CENTER>
<TABLE WIDTH=75% BORDER=0 CELLSPACING=0 CELLPADDING=1>
<TR><TD BGCOLOR=#000000>
<TABLE WIDTH=100% BORDER=0 CELLSPACING=0 CELLPADDING=10>
<TR><TD BGCOLOR=#EEEEEE>";

$setup_footer = "
</TD></TR></TABLE>
</TD></TR></TABLE>
</CENTER>
</BODY></HTML>";






// ensure that magic quotes are OFF
// we hand-filter all _REQUEST data with regexs before submitting it to the DB
if( get_magic_quotes_gpc() ) {
    // force magic quotes to be removed
    $_GET     = array_map( 'ag_stripslashes_deep', $_GET );
    $_POST    = array_map( 'ag_stripslashes_deep', $_POST );
    $_REQUEST = array_map( 'ag_stripslashes_deep', $_REQUEST );
    $_COOKIE  = array_map( 'ag_stripslashes_deep', $_COOKIE );
    }
    


// Check that the referrer header is this page, or kill the connection.
// Used to block XSRF attacks on state-changing functions.
// (To prevent it from being dangerous to surf other sites while you are
// logged in as admin.)
// Thanks Chris Cowan.
function ag_checkReferrer() {
    global $fullServerURL;
    
    if( !isset($_SERVER['HTTP_REFERER']) ||
        strpos($_SERVER['HTTP_REFERER'], $fullServerURL) !== 0 ) {
        
        die( "Bad referrer header" );
        }
    }




// all calls need to connect to DB, so do it once here
ag_connectToDatabase();

// close connection down below (before function declarations)


// testing:
//sleep( 5 );


// general processing whenver server.php is accessed directly




// grab POST/GET variables
$action = ag_requestFilter( "action", "/[A-Z_]+/i" );

$debug = ag_requestFilter( "debug", "/[01]/" );

$remoteIP = "";
if( isset( $_SERVER[ "REMOTE_ADDR" ] ) ) {
    $remoteIP = $_SERVER[ "REMOTE_ADDR" ];
    }




if( $action == "version" ) {
    global $ag_version;
    echo "$ag_version";
    }
else if( $action == "get_sequence_number" ) {
    ag_getSequenceNumber();
    }
else if( $action == "check_grant" ) {
    ag_checkGrant();
    }
else if( $action == "grant" ) {
    ag_grant();
    }
else if( $action == "manual_grant" ) {
    ag_manualGrant();
    }
else if( $action == "register_vote" ) {
    ag_registerVote();
    }
else if( $action == "register_github" ) {
    ag_registerGithub();
    }
else if( $action == "get_content_leader" ) {
    ag_getContentLeader();
    }
else if( $action == "show_account" ) {
    ag_showAccount();
    }
else if( $action == "show_log" ) {
    ag_showLog();
    }
else if( $action == "clear_log" ) {
    ag_clearLog();
    }
else if( $action == "show_data" ) {
    ag_showData();
    }
else if( $action == "show_detail" ) {
    ag_showDetail();
    }
else if( $action == "delete_user" ) {
    ag_deleteUser();
    }
else if( $action == "logout" ) {
    ag_logout();
    }
else if( $action == "add_steam_gift_keys" ) {
    ag_addSteamGiftKeys();
    }
else if( $action == "ag_setup" ) {
    global $setup_header, $setup_footer;
    echo $setup_header; 

    echo "<H2>AHAP Gate Server Web-based Setup</H2>";

    echo "Creating tables:<BR>";

    echo "<CENTER><TABLE BORDER=0 CELLSPACING=0 CELLPADDING=1>
          <TR><TD BGCOLOR=#000000>
          <TABLE BORDER=0 CELLSPACING=0 CELLPADDING=5>
          <TR><TD BGCOLOR=#FFFFFF>";

    ag_setupDatabase();

    echo "</TD></TR></TABLE></TD></TR></TABLE></CENTER><BR><BR>";
    
    echo $setup_footer;
    }
else if( preg_match( "/server\.php/", $_SERVER[ "SCRIPT_NAME" ] ) ) {
    // server.php has been called without an action parameter

    // the preg_match ensures that server.php was called directly and
    // not just included by another script
    
    // quick (and incomplete) test to see if we should show instructions
    global $tableNamePrefix;
    
    // check if our tables exist
    $exists =
        ag_doesTableExist( $tableNamePrefix . "server_globals" ) &&
        ag_doesTableExist( $tableNamePrefix . "users" ) &&
        ag_doesTableExist( $tableNamePrefix . "log" ) &&
        ag_doesTableExist( $tableNamePrefix . "steam_key_bank" );
    
        
    if( $exists  ) {
        echo "AHAP Gate Server database setup and ready";
        }
    else {
        // start the setup procedure

        global $setup_header, $setup_footer;
        echo $setup_header; 

        echo "<H2>AHAP Gate Server Web-based Setup</H2>";
    
        echo "AHAP Gate Server will walk you through a " .
            "brief setup process.<BR><BR>";
        
        echo "Step 1: ".
            "<A HREF=\"server.php?action=ag_setup\">".
            "create the database tables</A>";

        echo $setup_footer;
        }
    }



// done processing
// only function declarations below

ag_closeDatabase();







/**
 * Creates the database tables needed by seedBlogs.
 */
function ag_setupDatabase() {
    global $tableNamePrefix;

    $tableName = $tableNamePrefix . "log";
    if( ! ag_doesTableExist( $tableName ) ) {

        // this table contains general info about the server
        // use INNODB engine so table can be locked
        $query =
            "CREATE TABLE $tableName(" .
            "entry TEXT NOT NULL, ".
            "entry_time DATETIME NOT NULL );";

        $result = ag_queryDatabase( $query );

        echo "<B>$tableName</B> table created<BR>";
        }
    else {
        echo "<B>$tableName</B> table already exists<BR>";
        }


    
    $tableName = $tableNamePrefix . "server_globals";
    if( ! ag_doesTableExist( $tableName ) ) {

        $query =
            "CREATE TABLE $tableName(" .
            "content_leader_email VARCHAR(254) NOT NULL );";

        $result = ag_queryDatabase( $query );

        echo "<B>$tableName</B> table created<BR>";

        $query = "INSERT INTO $tableName ".
            "SET content_leader_email = '';";
        ag_queryDatabase( $query );
        }
    else {
        echo "<B>$tableName</B> table already exists<BR>";
        }

    
    $tableName = $tableNamePrefix . "users";
    if( ! ag_doesTableExist( $tableName ) ) {

        $query =
            "CREATE TABLE $tableName(" .
            "id INT UNSIGNED NOT NULL PRIMARY KEY AUTO_INCREMENT," .
            "email VARCHAR(254) NOT NULL," .
            "UNIQUE KEY( email )," .
            "sequence_number INT NOT NULL," .
            "github_username VARCHAR(254) NOT NULL," .
            "content_leader_email_vote VARCHAR(254) NOT NULL," .
            "steam_gift_key VARCHAR(254) NOT NULL," .
            "grant_time DATETIME NOT NULL, ".
            "last_vote_time DATETIME NOT NULL );";

        $result = ag_queryDatabase( $query );

        echo "<B>$tableName</B> table created<BR>";
        }
    else {
        echo "<B>$tableName</B> table already exists<BR>";
        }


    $tableName = $tableNamePrefix . "steam_key_bank";
    if( ! ag_doesTableExist( $tableName ) ) {

        // bank of keys not assigned yet
        $query =
            "CREATE TABLE $tableName(" .
            "steam_gift_key VARCHAR(255) NOT NULL PRIMARY KEY, ".
            "add_time DATETIME ) ".
            "ENGINE = INNODB;";

        $result = ag_queryDatabase( $query );

        echo "<B>$tableName</B> table created<BR>";
        }
    else {
        echo "<B>$tableName</B> table already exists<BR>";
        }

    }



function ag_showLog() {
    ag_checkPassword( "show_log" );

    echo "[<a href=\"server.php?action=show_data" .
        "\">Main</a>]<br><br><br>";
    
    global $tableNamePrefix;

    $query = "SELECT * FROM $tableNamePrefix"."log ".
        "ORDER BY entry_time DESC;";
    $result = ag_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );



    echo "<a href=\"server.php?action=clear_log\">".
        "Clear log</a>";
        
    echo "<hr>";
        
    echo "$numRows log entries:<br><br><br>\n";
        

    for( $i=0; $i<$numRows; $i++ ) {
        $time = ag_mysqli_result( $result, $i, "entry_time" );
        $entry = htmlspecialchars( ag_mysqli_result( $result, $i, "entry" ) );

        echo "<b>$time</b>:<br>$entry<hr>\n";
        }
    }



function ag_clearLog() {
    ag_checkPassword( "clear_log" );

     echo "[<a href=\"server.php?action=show_data" .
         "\">Main</a>]<br><br><br>";
    
    global $tableNamePrefix;

    $query = "DELETE FROM $tableNamePrefix"."log;";
    $result = ag_queryDatabase( $query );
    
    if( $result ) {
        echo "Log cleared.";
        }
    else {
        echo "DELETE operation failed?";
        }
    }




















function ag_logout() {
    ag_checkReferrer();
    
    ag_clearPasswordCookie();

    echo "Logged out";
    }




function ag_showData( $checkPassword = true ) {
    // these are global so they work in embeded function call below
    global $skip, $search, $order_by;

    if( $checkPassword ) {
        ag_checkPassword( "show_data" );
        }
    
    global $tableNamePrefix, $remoteIP;
    
    $keysLeftInBank = ag_countKeysInBank();
    $leader = ag_getContentLeaderInternal();

    $leader_github = ag_getGithubUsername( $leader );
    
    echo "<table width='100%' border=0><tr>".
        "<td valign=top>[<a href=\"server.php?action=show_data" .
            "\">Main</a>]</td>".
        "<td  valign=top align=center>".
          "<b>$keysLeftInBank</b> unassigned keys remain".
          "<br><br>Current Content Leader: $leader<br>".
                  "(Github: $leader_github)</td>".
        "<td valign=top align=right>[<a href=\"server.php?action=logout" .
            "\">Logout</a>]</td>".
        "</tr></table><br><br><br>";




    $skip = ag_requestFilter( "skip", "/[0-9]+/", 0 );
    
    global $usersPerPage;
    
    $search = ag_requestFilter( "search", "/[A-Z0-9_@. \-]+/i" );

    $order_by = ag_requestFilter( "order_by", "/[A-Z_]+/i",
                                  "id" );
    
    $keywordClause = "";
    $searchDisplay = "";
    
    if( $search != "" ) {
        

        $keywordClause = "WHERE ( email LIKE '%$search%' " .
            "OR id LIKE '%$search%' ".
            "OR github_username LIKE '%$search%' ".
            "OR content_leader_email_vote LIKE '%$search%' ".
            "OR steam_gift_key LIKE '%$search%' ".
            " ) ";

        $searchDisplay = " matching <b>$search</b>";
        }
    

    

    // first, count results
    $query = "SELECT COUNT(*) FROM $tableNamePrefix".
        "users $keywordClause;";

    $result = ag_queryDatabase( $query );
    $totalRecords = ag_mysqli_result( $result, 0, 0 );


    $orderDir = "DESC";

    if( $order_by == "email" ) {
        $orderDir = "ASC";
        }
    
             
    $query = "SELECT * ".
        "FROM $tableNamePrefix"."users $keywordClause".
        "ORDER BY $order_by $orderDir ".
        "LIMIT $skip, $usersPerPage;";
    $result = ag_queryDatabase( $query );
    
    $numRows = mysqli_num_rows( $result );

    $startSkip = $skip + 1;
    
    $endSkip = $startSkip + $usersPerPage - 1;

    if( $endSkip > $totalRecords ) {
        $endSkip = $totalRecords;
        }



        // form for searching users and resetting tokens
?>
        <hr>
             <table border=0 width=100%><tr><td>
            <FORM ACTION="server.php" METHOD="post">
    <INPUT TYPE="hidden" NAME="action" VALUE="show_data">
    <INPUT TYPE="hidden" NAME="order_by" VALUE="<?php echo $order_by;?>">
    <INPUT TYPE="text" MAXLENGTH=40 SIZE=20 NAME="search"
             VALUE="<?php echo $search;?>">
    <INPUT TYPE="Submit" VALUE="Search">
    </FORM>
             </td>
             </tr>
             </table>
        <hr>
<?php

    

    
    echo "$totalRecords user records". $searchDisplay .
        " (showing $startSkip - $endSkip):<br>\n";

    
    $nextSkip = $skip + $usersPerPage;

    $prevSkip = $skip - $usersPerPage;
    
    if( $prevSkip >= 0 ) {
        echo "[<a href=\"server.php?action=show_data" .
            "&skip=$prevSkip&search=$search&order_by=$order_by\">".
            "Previous Page</a>] ";
        }
    if( $nextSkip < $totalRecords ) {
        echo "[<a href=\"server.php?action=show_data" .
            "&skip=$nextSkip&search=$search&order_by=$order_by\">".
            "Next Page</a>]";
        }

    echo "<br><br>";
    
    echo "<table border=1 cellpadding=5>\n";

    function orderLink( $inOrderBy, $inLinkText ) {
        global $skip, $search, $order_by;
        if( $inOrderBy == $order_by ) {
            // already displaying this order, don't show link
            return "<b>$inLinkText</b>";
            }

        // else show a link to switch to this order
        return "<a href=\"server.php?action=show_data" .
            "&search=$search&skip=$skip&order_by=$inOrderBy\">$inLinkText</a>";
        }

    
    echo "<tr>\n";    
    echo "<tr><td>".orderLink( "id", "ID" )."</td>\n";
    echo "<td>".orderLink( "email", "Email" )."</td>\n";
    echo "<td>".orderLink( "github_username", "Github Username" )."</td>\n";
    echo "<td>".orderLink( "content_leader_email_vote",
                           "Chosen Leader" )."</td>\n";
    echo "<td>".orderLink( "steam_gift_key",
                           "Steam Key" )."</td>\n";
    echo "<td>".orderLink( "grant_time", "Grant Time" )."</td>\n";
    echo "<td>".orderLink( "last_vote_time", "Last Vote Time" )."</td>\n";
    echo "</tr>\n";


    for( $i=0; $i<$numRows; $i++ ) {
        $id = ag_mysqli_result( $result, $i, "id" );
        $email = ag_mysqli_result( $result, $i, "email" );
        $github_username = ag_mysqli_result( $result, $i, "github_username" );
        $content_leader_email_vote =
            ag_mysqli_result( $result, $i, "content_leader_email_vote" );
        $steam_gift_key = ag_mysqli_result( $result, $i, "steam_gift_key" );
        $grant_time = ag_mysqli_result( $result, $i, "grant_time" );
        $last_vote_time = ag_mysqli_result( $result, $i, "last_vote_time" );

        
        $encodedEmail = urlencode( $email );

        
        echo "<tr>\n";

        echo "<td>$id</td>\n";
        echo "<td>".
            "<a href=\"server.php?action=show_detail&email=$encodedEmail\">".
            "$email</a></td>\n";
        echo "<td>$github_username</td>\n";
        echo "<td>$content_leader_email_vote</td>\n";
        echo "<td>$steam_gift_key</td>\n";
        echo "<td>$grant_time</td>\n";
        echo "<td>$last_vote_time</td>\n";
        echo "</tr>\n";
        }
    echo "</table>";


    echo "<hr>";

    
?>
    <FORM ACTION="server.php" METHOD="post">
    <INPUT TYPE="hidden" NAME="action" VALUE="add_steam_gift_keys">
    Add Steam Gift Keys:<br>
    (One per line)<br>

         <TEXTAREA NAME="steam_gift_keys" COLS=50 ROWS=10></TEXTAREA><br>
    <INPUT TYPE="checkbox" NAME="confirm" VALUE=1> Confirm<br>      
    <INPUT TYPE="Submit" VALUE="Add">
    </FORM>
    <hr>


<?php


echo "<hr>";

    
?>
    <FORM ACTION="server.php" METHOD="post">
    <INPUT TYPE="hidden" NAME="action" VALUE="manual_grant">
    Grant for email:<br>
    <INPUT TYPE="text" NAME="email" size=40>
    <INPUT TYPE="Submit" VALUE="Grant">
    </FORM>
    <hr>


<?php



    
    echo "<a href=\"server.php?action=show_log\">".
        "Show log</a>";
    echo "<hr>";
    echo "Generated for $remoteIP\n";

    }







function ag_showDetail( $checkPassword = true ) {
    if( $checkPassword ) {
        ag_checkPassword( "show_detail" );
        }
    
    echo "[<a href=\"server.php?action=show_data" .
         "\">Main</a>]<br><br><br>";
    
    global $tableNamePrefix;
    

    $email = ag_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i" );
            
    $query = "SELECT id, github_username, content_leader_email_vote ".
        "FROM $tableNamePrefix"."users ".
        "WHERE email = '$email';";
    $result = ag_queryDatabase( $query );

    $id = ag_mysqli_result( $result, 0, "id" );
    $github_username = ag_mysqli_result( $result, 0, "github_username" );
    $content_leader_email_vote =
        ag_mysqli_result( $result, 0, "content_leader_email_vote" );
    
    echo "<center><table border=0><tr><td>";
    
    echo "<b>ID:</b> $id<br><br>";
    echo "<b>Email:</b> $email<br><br>";
    echo "<b>Github Username:</b> $github_username<br><br>";
    echo "<b>Content Leader:</b> $content_leader_email_vote<br><br>";
    echo "<br><br>";
?>
    <hr>
    Delete User:<br>
    <FORM ACTION="server.php" METHOD="post">
    <INPUT TYPE="hidden" NAME="action" VALUE="delete_user">
    <INPUT TYPE="hidden" NAME="email" VALUE="<?php echo $email;?>">
    <INPUT TYPE="checkbox" NAME="confirm" VALUE=1> Confirm        
    <INPUT TYPE="Submit" VALUE="Delete">
    </FORM>
    <hr>
<?php
    }



function ag_deleteUser() {
    ag_checkPassword( "delete_user" );

    $email = ag_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i" );
    $confirm = ag_requestFilter( "confirm", "/[01]/", "0" );

    if( $confirm == 0 ) {
        echo "Must check confirmation box to delete user<hr>";

        ag_showDetail( false );
        return;
        }

    // confirm checked


    global $tableNamePrefix;
    
    $query = "DELETE FROM $tableNamePrefix". "users " .
        "WHERE email = '$email';";
    ag_queryDatabase( $query );



    // there is no way to ungrant on Steam
    // but do un-map them


    // find their ticket IDs
    global $tableNamePrefixAHAPTicketServer;

    $query = "SELECT ticket_id FROM ".
        "$tableNamePrefixAHAPTicketServer". "tickets ".
        "WHERE email = '$email'";
    
    $result = ag_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );

    for( $i=0; $i<$numRows; $i++ ) {

        $ticket_id = ag_mysqli_result( $result, $i, "ticket_id" );

        global $tableNamePrefixAHAPSteamGateServer;
            
        $query =
            "DELETE FROM $tableNamePrefixAHAPSteamGateServer"."mapping ".
            "WHERE ticket_id = '$ticket_id';";
        
        ag_queryDatabase( $query );
        }

    
    // now clear their ticketServer tickets
    
    $query = "DELETE FROM $tableNamePrefixAHAPTicketServer". "tickets ".
            "WHERE email = '$email'";
    
    ag_queryDatabase( $query );


    echo "User $email deleted.<hr>";

    ag_showData( false );
    }




function ag_getSequenceNumber() {
    global $tableNamePrefix;
    

    $email = ag_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i", "" );

    if( $email == "" ) {
        ag_log( "getSequenceNumber denied for bad email" );

        echo "DENIED";
        return;
        }
    
    
    $seq = ag_getSequenceNumberForEmail( $email );

    echo "$seq\n"."OK";
    }



// assumes already-filtered, valid email
// returns 0 if not found
function ag_getSequenceNumberForEmail( $inEmail ) {
    global $tableNamePrefix;
    
    $query = "SELECT sequence_number FROM $tableNamePrefix"."users ".
        "WHERE email = '$inEmail';";
    $result = ag_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );

    if( $numRows < 1 ) {
        return 0;
        }
    else {
        return ag_mysqli_result( $result, 0, "sequence_number" );
        }
    }



// $concatData will be concatonated with sequence number when
// computing hash, can be ""
//
// If no record exists for email, 0 is returned
// If record exists for email, a value > 0 will be returned
function ag_checkSeqHash( $email, $concatData ) {
    global $sharedGameServerSecret;


    global $action;


    $sequence_number = ag_requestFilter( "sequence_number", "/[0-9]+/i", "0" );

    $hash_value = ag_requestFilter( "hash_value", "/[A-F0-9]+/i", "" );

    $hash_value = strtoupper( $hash_value );


    if( $email == "" ) {

        ag_log( "$action token denied for bad email" );
        
        echo "DENIED";
        die();
        }
    
    $trueSeq = ag_getSequenceNumberForEmail( $email );

    if( $trueSeq > $sequence_number ) {
        ag_log( "$action denied for stale sequence number ".
                "$sequence_number, current = $trueSeq" );

        echo "DENIED";
        die();
        }

    $computedHashValue =
        strtoupper( ag_hmac_sha1( $sharedGameServerSecret,
                                  $sequence_number . $concatData ) );

    if( $computedHashValue != $hash_value ) {
        ag_log( "$action denied for bad hash value on ".
                "'". $sequence_number . $concatData . "' ".
                "(correct = $computedHashValue )" );

        echo "DENIED";
        die();
        }

    return $trueSeq;
    }






// copied from steamGate
//
// Checks ownership of $steamAppID (from settings.php)
// returns the steamID of the true owner if they have access to it (may be
// ID of family member)
// returns 0 if they don't own it at all.
function ag_doesSteamUserOwnApp( $inSteamID ) {
    global $steamAppID, $steamWebAPIKey;
    
    $url = "https://partner.steam-api.com/ISteamUser/CheckAppOwnership/V0001".
        "?format=xml".
        "&key=$steamWebAPIKey".
        "&steamid=$inSteamID".
        "&appid=$steamAppID";

    $result = file_get_contents( $url );

    
    $matched = preg_match( "#<ownsapp>(\w+)</ownsapp>#",
                           $result, $matches );

    if( $matched && $matches[1] == "true" ) {

        // make sure we return the true owner
        $matchedB = preg_match( "#<ownersteamid>(\d+)</ownersteamid>#",
                                $result, $matchesB );

        if( $matchedB ) {
            return $matchesB[1];
            }
        else {
            return 0;
            }
        }
    else {
        return 0;
        }
    }



// copied from steamGate
//
// Uses GrantPackage API to grant
// returns steamID of app owner on success, 0 on failure
function ag_grantPackage( $inSteamID ) {
    global $steamAppID, $steamWebAPIKey, $packageID, $remoteIP;

    /*
      $url = "https://partner.steam-api.com/ISteamUser/GrantPackage/v1".
      "?format=xml".
      "&key=$steamWebAPIKey".
      "&steamid=$inSteamID".
      "&packageid=$packageID".
      "&ipaddress=$remoteIP";
      
      $result = file_get_contents( $url );
    */


    $clientIP = $remoteIP;

    // Valve requiers ipv4 address in GrantPackage

    if( ! filter_var( $clientIP, FILTER_VALIDATE_IP, FILTER_FLAG_IPV4 ) ) {
        global $defaultClientIP;

        ag_log( "Got non-ipv4 client address $clientIP, ".
                "replacing with default $defaultClientIP" );
        $clientIP = $defaultClientIP;
        }
    
    
    // GrantPackage requires POST

    $postData = http_build_query(
        array(
            'format' => 'xml',
            'key' => $steamWebAPIKey,
            'steamid' => $inSteamID,
            'packageid' => $packageID,
            'ipaddress' => $clientIP ) );

    $opts = array(
        'http' =>
        array(
            'method'  => 'POST',
            'header'  => 'Content-type: application/x-www-form-urlencoded',
            'content' => $postData ) );

    $context  = stream_context_create( $opts );

    $url = "https://partner.steam-api.com/ISteamUser/GrantPackage/v1";
    
    $result = file_get_contents(
        $url,
        false, $context );

    // not sure about format of results from GrantPackage

    
    // instead, just turn around immediately and check for ownership
    $ownsAppNow = ag_doesSteamUserOwnApp( $inSteamID );

    if( $ownsAppNow == 0 ) {
        // granting failed

        $header = $http_response_header[0];
        ag_log( "GrantPackage failed.  URL $url  ".
                "POST data: '$postData'  ".
                "Result header:  '$header'  ".
                "Result body:  '$result'" );
        }
    else {
        ag_log( "GrantPackage success for $inSteamID from ".
                "IP $clientIP ($remoteIP)" );
        }
    
    return $ownsAppNow;
    }



function ag_manualGrant() {
    ag_checkPassword( "manual_grant" );
    
    $email = ag_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i", "" );

    echo "[<a href=\"server.php?action=show_data" .
        "\">Main</a>]<br><br><br>";
    
    $seq = ag_getSequenceNumberForEmail( $email );

    if( $seq != 0 ) {
        echo "Grant for $email already exists.";
        return;
        }

    echo "<pre>";
    
    ag_grantForNew( $email );
    
    echo "</pre>";
    }




function ag_grantForNew( $email ) {
    global $tableNamePrefix;
    
    // check if OHOL ticket exists
    global $tableNamePrefixOHOLTicketServer;
    $query = "SELECT ticket_id, name, email_opt_in, tag, order_number ".
        "FROM $tableNamePrefixOHOLTicketServer"."tickets " .
        "WHERE email = '$email';";


    $result = ag_queryDatabase( $query );
        
    $numRows = mysqli_num_rows( $result );
        
    $ticket_id = "";
    $email_opt_in = 0;
    $tag = "";
    $order_number = "";
    $name = "";
        
    // could be more than one with this email
    // return first only
    if( $numRows > 0 ) {
        $ticket_id = ag_mysqli_result( $result, 0, "ticket_id" );
        $email_opt_in = ag_mysqli_result( $result, 0, "email_opt_in" );
        $tag = ag_mysqli_result( $result, 0, "tag" );
        $order_number = ag_mysqli_result( $result, 0, "order_number" );
        $name = ag_mysqli_result( $result, 0, "name" );
        }
    else {
        ag_log( "grant denied because $email not found in ticketServer" );
            
        echo "DENIED";
        return;
        }
        

        
    // create AHAP ticket
    // IGNORE if it already exists
    global $tableNamePrefixAHAPTicketServer;
        
    $query = "INSERT IGNORE INTO ".
        "$tableNamePrefixAHAPTicketServer". "tickets ".
        "VALUES ( " .
        "'$ticket_id', CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, ".
        "'$name', '$email', '$order_number', ".
        "'$tag', '', '0', '0', '0', " .
        "'$email_opt_in' );";

    ag_queryDatabase( $query );

        
    $steamID = "";
        
    if( preg_match( "/@steamgames\.com/", $email ) ) {
        $steamID = preg_replace( '/@steamgames\.com/', '', $email, 1 );
        }
    else {
        // they don't have a steam-ID placeholder email

        // try looking them up in the steamGate mapping
        global $tableNamePrefixOHOLSteamGateServer;
            
        $query = "SELECT steam_id FROM ".
            "$tableNamePrefixOHOLSteamGateServer". "mapping ".
            "WHERE ticket_id = '$ticket_id';";

        $result = ag_queryDatabase( $query );

        $numRows = mysqli_num_rows( $result );

        if( $numRows > 0 ) {
            $steamID = ag_mysqli_result( $result, 0, "steam_id" );

            }
        }
        
            
    if( $steamID != "" ) {
        // try to make a new mapping for them in the
        // AHAP-specific steamGate
        global $tableNamePrefixAHAPSteamGateServer;
            
        $query =
            "INSERT IGNORE INTO $tableNamePrefixAHAPSteamGateServer".
            "mapping( steam_id, ticket_id, steam_gift_key, ".
            "         creation_date ) ".
            "VALUES( '$steamID', ".
            "        '$ticket_id', '', CURRENT_TIMESTAMP );";
        
        ag_queryDatabase( $query );
        }
        


    // get a steam key for them
    // (this can be "" if we run out of keys)

    $steam_gift_key = ag_getSteamKey();
        

        
    // finally, create a record for this user here
    $query = "INSERT INTO $tableNamePrefix". "users SET " .
        "email = '$email', ".
        "github_username = '', ".
        "content_leader_email_vote = '', ".
        "steam_gift_key = '$steam_gift_key', ".
        "sequence_number = 1, ".
        "grant_time = CURRENT_TIMESTAMP, ".
        "last_vote_time = CURRENT_TIMESTAMP ".
        "ON DUPLICATE KEY UPDATE sequence_number = sequence_number + 1 ;";
    ag_queryDatabase( $query );

    if( $steam_gift_key == "" ) {
        echo "NO-KEYS-LEFT\n";
        }
    else {
        echo "$steam_gift_key\n";
        }
        
    global $fullServerURL;
        
    echo "$fullServerURL".
        "?action=show_account&ticket_id=$ticket_id\n";
        
    }





function ag_grant() {
    global $tableNamePrefix;

    $email = ag_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i", "" );


    // concat email with sequence number, to prevent replaying grant
    // action with seq of 0 for different emails
    $trueSeq = ag_checkSeqHash( $email, strtolower( $email ) );
    
    
    if( $trueSeq == 0 ) {
        // no record exists, grant can go through

        ag_grantForNew( $email );
        
        echo "OK";
        return;
        }
    else {
        // record already exists for this email, can't re-grant

        // but go ahead and return existing info


        // check if ahap ticket exists
        global $tableNamePrefixAHAPTicketServer;
        $query = "SELECT ticket_id ".
            "FROM $tableNamePrefixAHAPTicketServer"."tickets " .
            "WHERE email = '$email';";


        $result = ag_queryDatabase( $query );
        
        $numRows = mysqli_num_rows( $result );
        
        $ticket_id = "";
        
        if( $numRows > 0 ) {
            $ticket_id = ag_mysqli_result( $result, 0, "ticket_id" );
            }
        else {
            ag_log( "repeat grant denied because $email not found in ".
                    "ahap ticketServer" );
            
            echo "DENIED";
            return;
            }

        $query = "SELECT steam_gift_key ".
            "FROM $tableNamePrefix"."users " .
            "WHERE email = '$email';";

                $result = ag_queryDatabase( $query );
        
        $numRows = mysqli_num_rows( $result );
        
        $steam_gift_key = "";
        
        if( $numRows > 0 ) {
            $steam_gift_key = ag_mysqli_result( $result, 0, "steam_gift_key" );
            }
        else {
            ag_log( "repeat grant denied because $email not found in ".
                    "ahap users table" );
            
            echo "DENIED";
            return;
            }

        // update sequence number
        $query = "UPDATE $tableNamePrefix"."users SET " .
            "sequence_number = sequence_number + 1 ".
            "WHERE email = '$email'; ";
        
        ag_queryDatabase( $query );


        if( $steam_gift_key == "" ) {
            echo "NO-KEYS-LEFT\n";
            }
        else {
            echo "$steam_gift_key\n";
            }
        
        global $fullServerURL;
        
        echo "$fullServerURL".
            "?action=show_account&ticket_id=$ticket_id\n";
        
        echo "OK";
        return;
        }
    }




function ag_registerVote() {
    global $tableNamePrefix;

    $email = ag_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i", "" );
    $leader_github = ag_requestFilter( "leader_github",
                                       "/[A-Z0-9\-]+/i", "" );

    $leader_github = strtolower( $leader_github );
    
    // include both email and github in hash check, so we can't replay voting
    // for a given leader, with a given sequence number for a different email
    $trueSeq = ag_checkSeqHash( $email,
                                strtolower( $email ) . $leader_github );
    
    
    if( $trueSeq == 0 ) {

        // no record of this player, can't vote
        ag_log( "grant denied because no record exists for voter $email" );
        
        echo "DENIED";
        return;
        }

    // make sure leader exists
    $query =
        "SELECT COUNT(*) from $tableNamePrefix"."users ".
        "WHERE github_username = '$leader_github';";

    $result = ag_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );

    if( $numRows < 1 ) {
        ag_log( "grant denied because counting records failed" );

        echo "DENIED";
        return;
        }

    if( ag_mysqli_result( $result, 0, 0 ) < 1 ) {
        // leader they are voting for doesn't exist
        ag_log( "grant denied because no record exists for leader w/ github ".
                $leader_github );

        echo "DENIED";
        return;
        }
    

    $leader_email = ag_getEmailForGithubUsername( $leader_github );
    
    // update the existing one
    $query = "UPDATE $tableNamePrefix"."users SET " .
        "sequence_number = sequence_number + 1, ".
        "content_leader_email_vote = '$leader_email', ".
        "last_vote_time = CURRENT_TIMESTAMP ".
        "WHERE email = '$email'; ";
    
    ag_queryDatabase( $query );

    ag_updateContentLeader();
    
    echo "OK";
    }



// $concatData will be concatonated with sequence number when
// computing hash, can be ""
//
// only works for emails that have records already, as created by the
// grant action
//
// returns valid email, on success
// prints DENIED and dies on failure
function ag_checkTicketServerSeqHash( $concatData ) {
    global $action;

    $email = ag_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i", "" );
    
    if( $email == "" ) {
        ag_log( "$action token denied for bad email" );
        
        echo "DENIED";
        die();
        }

    $email = strtolower( $email );
    
    $sequence_number = ag_requestFilter( "sequence_number", "/[0-9]+/i", "0" );

    $trueSeq = ag_getSequenceNumberForEmail( $email );

    if( $trueSeq == 0 ) {
        ag_log( "$action token denied because no record exists for email" );
        
        echo "DENIED";
        die();
        }
    
    
    if( $trueSeq > $sequence_number ) {
        ag_log( "$action denied for stale sequence number" );

        echo "DENIED";
        die();
        }
    
    
    // we check this sequence number against the ticket server API
    global $ahapTicketServerURL;
    
    

    $hash_value = ag_requestFilter( "hash_value", "/[A-F0-9]+/i", "" );

    $hash_value = strtoupper( $hash_value );

    $encodedEmail = urlencode( $email );

    $stringToHash = $sequence_number . $concatData;
    
    $result = trim( file_get_contents(
                        "$ahapTicketServerURL".
                        "?action=check_ticket_hash".
                        "&email=$encodedEmail".
                        "&hash_value=$hash_value".
                        "&string_to_hash=$stringToHash" ) );

    if( $result != "VALID" ) {
        ag_log( "$action denied for bad hash value $hash_value, ".
                "string_to_hash = $stringToHash" );

        echo "DENIED";
        die();
        }

    return $email;
    }

    


function ag_registerGithub() {
    global $tableNamePrefix;

    $github_username = strtolower( ag_requestFilter( "github_username",
                                                     "/[A-Z0-9\-]+/i", "" ) );

    // will die on failure
    $email = ag_checkTicketServerSeqHash( $github_username );
    

    $oldGithubUsername = ag_getGithubUsername( $email );


    $leader = ag_getContentLeaderInternal();

    if( $oldGithubUsername != "" &&
        $oldGithubUsername != $github_username &&
        $leader == $email ) {
        // we are the leader
        // and our github email has changed

        ag_ungrantGithub( $oldGithubUsername );

        ag_grantGithub( $github_username );
        }
        
    
    $query = "UPDATE $tableNamePrefix"."users SET " .
        "sequence_number = sequence_number + 1, ".
        "github_username = '$github_username' ".
        "WHERE email = '$email'; ";
    
    ag_queryDatabase( $query );
    
    echo "OK";
    }



function ag_getContentLeaderInternal() {
    global $tableNamePrefix;
    
    $query = "SELECT content_leader_email " .
        "FROM $tableNamePrefix"."server_globals ;";

    $result = ag_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );

    if( $numRows < 1 ) {
        return "";
        }
    $leader = ag_mysqli_result( $result, 0, "content_leader_email" );

    return $leader;
    }



function ag_getContentLeader() {
    global $tableNamePrefix;

    // will die on failure
    $email = ag_checkTicketServerSeqHash();

    
    ag_updateContentLeader();
    

    // got here, hash check succeeded  

    $leader_email = ag_getContentLeaderInternal();

    $leader_github = ag_getGithubUsername( $leader_email );
    
    
    // update sequence number
    $query = "UPDATE $tableNamePrefix"."users SET " .
        "sequence_number = sequence_number + 1 ".
        "WHERE email = '$email'; ";
    
    ag_queryDatabase( $query );
    
    echo "$leader_github\n";
    echo "OK";
    }



function ag_showAccount() {
    $ticket_id = ag_requestFilter( "ticket_id", "/[A-HJ-NP-Z2-9\-]+/i" );


    $ticket_id = strtoupper( $ticket_id );

    global $tableNamePrefixAHAPTicketServer;

    $query = "SELECT email FROM ".
        "$tableNamePrefixAHAPTicketServer". "tickets ".
        "WHERE ticket_id = '$ticket_id'";
    
    $result = ag_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );

    if( $numRows == 0 ) {
        echo "Ticket ID '$ticket_id' not found";
        return;
        }

    $email = ag_mysqli_result( $result, 0, "email" );


    global $tableNamePrefix;

    $query = "SELECT steam_gift_key FROM $tableNamePrefix"."users ".
        "WHERE email = '$email';";
    
    $result = ag_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );

    if( $numRows == 0 ) {
        echo "Grant record not found for $email";
        return;
        }

    $steam_gift_key = ag_mysqli_result( $result, 0, "steam_gift_key" );

    global $header, $footer;


    eval( $header );

    echo "<center><br><br>";
    
    echo "Your Another Planet Steam key:<br><br>$steam_gift_key<br><br><br>";

    global $ahapTicketServerURL;

    $url =
        $ahapTicketServerURL . "?action=show_downloads&ticket_id=$ticket_id";
    
    
    echo "Your Another Planet off-Steam downloads:<br><br>".
        "<a href='$url'>$url</a><br><br><br>";
    
    echo "</center>";

    eval( $footer );
    }




// returns "" if user for $inEmail does not exist
function ag_getGithubUsername( $inEmail ) {
    global $tableNamePrefix;
    
    $query =
        "SELECT github_username ".
        "FROM $tableNamePrefix"."users ".
        "WHERE email='$inEmail';";
            
    $result = ag_queryDatabase( $query );
            
    $numRows = mysqli_num_rows( $result );
    
    $github_username = "";
            
    if( $numRows > 0 ) {
        $github_username = ag_mysqli_result( $result, 0, "github_username" );
        }
            
    return $github_username;
    }


// returns "" if user for $inGithub does not exist
function ag_getEmailForGithubUsername( $inGithub ) {
    global $tableNamePrefix;
    
    $query =
        "SELECT email ".
        "FROM $tableNamePrefix"."users ".
        "WHERE github_username='$inGithub';";
            
    $result = ag_queryDatabase( $query );
            
    $numRows = mysqli_num_rows( $result );
    
    $email = "";
            
    if( $numRows > 0 ) {
        $email = ag_mysqli_result( $result, 0, "email" );
        }
            
    return $email;
    }



// re-compute vote based on people who voted in past week
// returns main email of leader
function ag_updateContentLeader() {

    global $tableNamePrefix;
    
    $query =
        "SELECT COUNT(*), content_leader_email_vote ".
        "FROM $tableNamePrefix"."users ".
        "WHERE last_vote_time >= DATE( NOW() - INTERVAL 7 DAY ) ".
        "GROUP BY content_leader_email_vote ".
        "ORDER BY COUNT(*) DESC;";

    $result = ag_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );

    $bestLeader = "";

    // highest votes first
    for( $i=0; $i<$numRows; $i++ ) {

        $leader_email =
            ag_mysqli_result( $result, $i, "content_leader_email_vote" );

        if( $leader_email != "" ) {
            $bestLeader = $leader_email;
            break;
            }
        }
    

    if( $bestLeader != "" ) {

        $oldLeader = ag_getContentLeaderInternal();

        if( $oldLeader != $bestLeader ) {

            // leader change

            $newGithubUsername = ag_getGithubUsername( $bestLeader );

            $oldGithubUsername = "";

            if( $oldLeader != "" ) {
                $oldGithubUsername = ag_getGithubUsername( $oldLeader );
                }
            
            if( $newGithubUsername != "" ) {

                // remove old leader from github
                ag_ungrantGithub( $oldGithubUsername );

                // add new leader to github
                ag_grantGithub( $newGithubUsername );
                }
            
            $query = "UPDATE $tableNamePrefix"."server_globals ".
                "SET content_leader_email = '$bestLeader';";
            ag_queryDatabase( $query );


            global $contentLeaderEmailFilePath;

            if( ! file_put_contents( $contentLeaderEmailFilePath,
                                     $bestLeader ) ) {
                ag_log( "Failed to write content leader email to file ".
                        $contentLeaderEmailFilePath );
                }
            }
        }

    return $bestLeader;
    }



function ag_grantGithub( $inGithubUsername ) {
    ag_log( "grantGithub on $inGithubUsername" );

    global $githubToken, $githubRepo;

    /*
    curl -L \
    -X PUT                                 \
    -H "Accept: application/vnd.github+json"    \
    -H "Authorization: Bearer <YOUR-TOKEN>"     \
    -H "X-GitHub-Api-Version: 2022-11-28"                        \
    https://api.github.com/repos/OWNER/REPO/collaborators/USERNAME  \
    -d '{"permission":"triage"}'
    */

    $opts = array(
        'http' =>
        array(
            'method'  => 'PUT',
            'header'  => array(
                'Accept: application/vnd.github+json',
                "Authorization: Bearer $githubToken",
                "X-GitHub-Api-Version: 2022-11-28",
                "User-Agent: curl/7.35.0" ),
            'content' => '{"permission":"push"}' ) );

    $context  = stream_context_create( $opts );


    $url = "https://api.github.com/repos/$githubRepo".
        "/collaborators/$inGithubUsername";

    $result = file_get_contents( $url, false, $context );
    }




function ag_deleteInviteGithub( $inGithubUsername ) {
    global $githubRepo, $githubToken;

    /*
    curl -L \
          -H "Accept: application/vnd.github+json" \
          -H "Authorization: Bearer <YOUR-TOKEN>" \
          -H "X-GitHub-Api-Version: 2022-11-28" \
        https://api.github.com/repos/OWNER/REPO/invitations
    */

    // list invitations
    
    $opts = array(
        'http' =>
        array(
            'method'  => 'GET',
            'header'  => array(
                'Accept: application/vnd.github+json',
                "Authorization: Bearer $githubToken",
                "X-GitHub-Api-Version: 2022-11-28",
                "User-Agent: curl/7.35.0" ) ) );

    $context  = stream_context_create( $opts );

    $result = file_get_contents(
        "https://api.github.com/repos/$githubRepo".
        "/invitations",
        false, $context );

    $a = json_decode( $result, true );

    $inviteID = "";
    
    foreach( $a as $invite ) {
        $username = $invite[ 'invitee' ][ 'login' ];

        if( strtolower( $username ) == strtolower( $inGithubUsername ) ) {
            $inviteID = $invite[ 'id' ];
            break;
            }
        }

    
    if( $inviteID != "" ) {

        /*
          curl -L \
          -X DELETE                                    \
          -H "Accept: application/vnd.github+json"      \
          -H "Authorization: Bearer <YOUR-TOKEN>"       \
          -H "X-GitHub-Api-Version: 2022-11-28"                         \
          https://api.github.com/repos/OWNER/REPO/invitations/INVITATION_ID
        */

        $opts = array(
            'http' =>
            array(
                'method'  => 'DELETE',
                'header'  => array(
                    'Accept: application/vnd.github+json',
                    "Authorization: Bearer $githubToken",
                    "X-GitHub-Api-Version: 2022-11-28",
                    "User-Agent: curl/7.35.0" ) ) );

        $context  = stream_context_create( $opts );
        
        $result = file_get_contents(
            "https://api.github.com/repos/$githubRepo".
            "/invitations/$inviteID",
            false, $context );
        }
    
    }



function ag_ungrantGithub( $inGithubUsername ) {
    ag_log( "ungrantGithub on $inGithubUsername" );
    
    global $githubToken, $githubRepo;
    
    /*
    curl -L \
    -X DELETE                                                       \
    -H "Accept: application/vnd.github+json"                        \
    -H "Authorization: Bearer <YOUR-TOKEN>"                         \
    -H "X-GitHub-Api-Version: 2022-11-28"                           \
    https://api.github.com/repos/OWNER/REPO/collaborators/USERNAME
    */

    $opts = array(
        'http' =>
        array(
            'method'  => 'DELETE',
            'header'  => array(
                'Accept: application/vnd.github+json',
                "Authorization: Bearer $githubToken",
                "X-GitHub-Api-Version: 2022-11-28",
                "User-Agent: curl/7.35.0" ) ) );

    $context  = stream_context_create( $opts );

    $result = file_get_contents(
        "https://api.github.com/repos/$githubRepo".
        "/collaborators/$inGithubUsername",
        false, $context );


    ag_deleteInviteGithub( $inGithubUsername );
    }



function ag_addSteamGiftKeys() {
    ag_checkPassword( "add_steam_gift_keys" );

    
    echo "[<a href=\"server.php?action=show_data" .
         "\">Main</a>]<br><br><br>";
    
    global $tableNamePrefix;

    $confirm = ag_requestFilter( "confirm", "/[01]/" );
    
    if( $confirm != 1 ) {
        echo "You must check the Confirm box to add keys\n";
        return;
        }
    

    $keys =
        ag_requestFilter( "steam_gift_keys", "/[A-Z0-9\- \n\r]+/" );

    

    $separateKeys = preg_split( "/\s+/", $keys );

    $goodKeys = array();

    foreach( $separateKeys as $k ) {
        // make sure key not already assigned to someone

        $query = "SELECT COUNT(*) FROM $tableNamePrefix"."users ".
            "WHERE steam_gift_key = '$k';";

        $result = ag_queryDatabase( $query );
        
        $hits = ag_mysqli_result( $result, 0, 0 );
        
        if( $hits == 0 ) {
            // unused
            $goodKeys[] = $k;
            }
        }
    

    $numKeys = count( $separateKeys );
    $numGoodKeys = count( $goodKeys );

    if( $numGoodKeys < $numKeys ) {
        $used = $numKeys - $numGoodKeys;
        echo "Adding <b>$numGoodKeys</b> (of $numKeys) new Steam gift keys ".
            "(<b>$used</b> already assigned)...<br>\n";
        }
    else {
        echo "Adding <b>$numGoodKeys</b> new Steam gift keys...<br>\n";
        }
    

    $query = "INSERT IGNORE INTO $tableNamePrefix"."steam_key_bank ".
        "VALUES ";

    $firstKey = true;
    
    foreach( $goodKeys as $key ) {
        if( $key != "" ) {
            
            if( ! $firstKey ) {
                $query = $query .", ";
                }
            
            $query = $query . " ('$key', CURRENT_TIMESTAMP )";
            
            $firstKey = false;
            }
        }


    if( $firstKey ) {
        echo "<br>No valid keys were provided.";
        return;
        }

    
    $query = $query . ";";
    

    $result = ag_queryDatabase( $query );

    global $ag_mysqlLink;
    $numInserted = mysqli_affected_rows( $ag_mysqlLink );

    
    echo "<br>Successfully added <b>$numInserted</b> keys.";

    if( $numInserted < $numGoodKeys ) {
        $numMissed = $numGoodKeys - $numInserted;
        echo "<br><br><font color=red>".
            "(Perhaps <b>$numMissed</b> were duplicates?)</font>";
        }
    }



function ag_countKeysInBank() {
    
    global $tableNamePrefix;
    
    $query = "SELECT COUNT(*) FROM $tableNamePrefix"."steam_key_bank;";
    
    $result = ag_queryDatabase( $query );
    
    $keysLeftInBank = ag_mysqli_result( $result, 0, 0 );

    return $keysLeftInBank;
    }



// gets a steam key from the steam_key_bank table atomically
// removing the key from the table
function ag_getSteamKey() {
    global $tableNamePrefix;
    
    ag_queryDatabase( "SET AUTOCOMMIT = 0;" );

    $query = "SELECT steam_gift_key FROM ".
        "$tableNamePrefix"."steam_key_bank ".
        "ORDER BY add_time ASC ".
        "LIMIT 1 ".
        "FOR UPDATE;";

    $result = ag_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );

    if( $numRows == 0 ) {
        ag_queryDatabase( "COMMIT;" );
        ag_queryDatabase( "SET AUTOCOMMIT = 1;" );

        ag_log( "There are no Steam keys left." );
        
        return "";
        }

    $steam_gift_key = ag_mysqli_result( $result, 0, 0 );
            
    $query = "DELETE FROM ".
        "$tableNamePrefix"."steam_key_bank ".
        "WHERE steam_gift_key = '$steam_gift_key';";

    ag_queryDatabase( $query );
    ag_queryDatabase( "COMMIT;" );
    ag_queryDatabase( "SET AUTOCOMMIT = 1;" );

    return $steam_gift_key;
    }

    





$ag_mysqlLink;


// general-purpose functions down here, many copied from seedBlogs

/**
 * Connects to the database according to the database variables.
 */  
function ag_connectToDatabase() {
    global $databaseServer,
        $databaseUsername, $databasePassword, $databaseName,
        $ag_mysqlLink;
    
    
    $ag_mysqlLink =
        mysqli_connect( $databaseServer, $databaseUsername, $databasePassword )
        or ag_operationError( "Could not connect to database server: " .
                              mysqli_error( $ag_mysqlLink ) );
    
    mysqli_select_db( $ag_mysqlLink, $databaseName )
        or ag_operationError( "Could not select $databaseName database: " .
                              mysqli_error( $ag_mysqlLink ) );
    }


 
/**
 * Closes the database connection.
 */
function ag_closeDatabase() {
    global $ag_mysqlLink;
    
    mysqli_close( $ag_mysqlLink );
    }




/**
 * Queries the database, and dies with an error message on failure.
 *
 * @param $inQueryString the SQL query string.
 *
 * @return a result handle that can be passed to other mysql functions.
 */
function ag_queryDatabase( $inQueryString ) {
    global $ag_mysqlLink;
    
    if( gettype( $ag_mysqlLink ) != "resource" ) {
        // not a valid mysql link?
        ag_connectToDatabase();
        }
    
    $result = mysqli_query( $ag_mysqlLink, $inQueryString );
    
    if( $result == FALSE ) {

        $errorNumber = mysqli_errno( $ag_mysqlLink );
        
        // server lost or gone?
        if( $errorNumber == 2006 ||
            $errorNumber == 2013 ||
            // access denied?
            $errorNumber == 1044 ||
            $errorNumber == 1045 ||
            // no db selected?
            $errorNumber == 1046 ) {

            // connect again?
            ag_closeDatabase();
            ag_connectToDatabase();

            $result = mysqli_query( $ag_mysqlLink, $inQueryString )
                or ag_operationError(
                    "Database query failed:<BR>$inQueryString<BR><BR>" .
                    mysqli_error( $ag_mysqlLink ) );
            }
        else {
            // some other error (we're still connected, so we can
            // add log messages to database
            ag_fatalError( "Database query failed:<BR>$inQueryString<BR><BR>" .
                           mysqli_error( $ag_mysqlLink ) );
            }
        }

    return $result;
    }



/**
 * Replacement for the old mysql_result function.
 */
function ag_mysqli_result( $result, $number, $field=0 ) {
    mysqli_data_seek( $result, $number );
    $row = mysqli_fetch_array( $result );
    return $row[ $field ];
    }



/**
 * Checks whether a table exists in the currently-connected database.
 *
 * @param $inTableName the name of the table to look for.
 *
 * @return 1 if the table exists, or 0 if not.
 */
function ag_doesTableExist( $inTableName ) {
    // check if our table exists
    $tableExists = 0;
    
    $query = "SHOW TABLES";
    $result = ag_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );


    for( $i=0; $i<$numRows && ! $tableExists; $i++ ) {

        $tableName = ag_mysqli_result( $result, $i, 0 );
        
        if( $tableName == $inTableName ) {
            $tableExists = 1;
            }
        }
    return $tableExists;
    }



function ag_log( $message ) {
    global $enableLog, $tableNamePrefix, $ag_mysqlLink;

    if( $enableLog ) {
        $slashedMessage = mysqli_real_escape_string( $ag_mysqlLink, $message );
    
        $query = "INSERT INTO $tableNamePrefix"."log VALUES ( " .
            "'$slashedMessage', CURRENT_TIMESTAMP );";
        $result = ag_queryDatabase( $query );
        }
    }



/**
 * Displays the error page and dies.
 *
 * @param $message the error message to display on the error page.
 */
function ag_fatalError( $message ) {
    //global $errorMessage;

    // set the variable that is displayed inside error.php
    //$errorMessage = $message;
    
    //include_once( "error.php" );

    // for now, just print error message
    $logMessage = "Fatal error:  $message";
    
    echo( $logMessage );

    ag_log( $logMessage );
    
    die();
    }



/**
 * Displays the operation error message and dies.
 *
 * @param $message the error message to display.
 */
function ag_operationError( $message ) {
    
    // for now, just print error message
    echo( "ERROR:  $message" );
    die();
    }


/**
 * Recursively applies the addslashes function to arrays of arrays.
 * This effectively forces magic_quote escaping behavior, eliminating
 * a slew of possible database security issues. 
 *
 * @inValue the value or array to addslashes to.
 *
 * @return the value or array with slashes added.
 */
function ag_addslashes_deep( $inValue ) {
    return
        ( is_array( $inValue )
          ? array_map( 'ag_addslashes_deep', $inValue )
          : addslashes( $inValue ) );
    }



/**
 * Recursively applies the stripslashes function to arrays of arrays.
 * This effectively disables magic_quote escaping behavior. 
 *
 * @inValue the value or array to stripslashes from.
 *
 * @return the value or array with slashes removed.
 */
function ag_stripslashes_deep( $inValue ) {
    return
        ( is_array( $inValue )
          ? array_map( 'ag_stripslashes_deep', $inValue )
          : stripslashes( $inValue ) );
    }



/**
 * Filters a $_REQUEST variable using a regex match.
 *
 * Returns "" (or specified default value) if there is no match.
 */
function ag_requestFilter( $inRequestVariable, $inRegex, $inDefault = "" ) {
    if( ! isset( $_REQUEST[ $inRequestVariable ] ) ) {
        return $inDefault;
        }

    return ag_filter( $_REQUEST[ $inRequestVariable ], $inRegex, $inDefault );
    }


/**
 * Filters a value  using a regex match.
 *
 * Returns "" (or specified default value) if there is no match.
 */
function ag_filter( $inValue, $inRegex, $inDefault = "" ) {
    
    $numMatches = preg_match( $inRegex,
                              $inValue, $matches );

    if( $numMatches != 1 ) {
        return $inDefault;
        }
        
    return $matches[0];
    }



// this function checks the password directly from a request variable
// or via hash from a cookie.
//
// It then sets a new cookie for the next request.
//
// This avoids storing the password itself in the cookie, so a stale cookie
// (cached by a browser) can't be used to figure out the password and log in
// later. 
function ag_checkPassword( $inFunctionName ) {
    $password = "";
    $password_hash = "";

    $badCookie = false;
    
    
    global $accessPasswords, $tableNamePrefix, $remoteIP, $enableYubikey,
        $passwordHashingPepper;

    $cookieName = $tableNamePrefix . "cookie_password_hash";

    $passwordSent = false;
    
    if( isset( $_REQUEST[ "passwordHMAC" ] ) ) {
        $passwordSent = true;

        // already hashed client-side on login form
        // hash again, because hash client sends us is not stored in
        // our settings file
        $password = ag_hmac_sha1( $passwordHashingPepper,
                                  $_REQUEST[ "passwordHMAC" ] );
        
        
        // generate a new hash cookie from this password
        $newSalt = time();
        $newHash = md5( $newSalt . $password );
        
        $password_hash = $newSalt . "_" . $newHash;
        }
    else if( isset( $_COOKIE[ $cookieName ] ) ) {
        ag_checkReferrer();
        $password_hash = $_COOKIE[ $cookieName ];
        
        // check that it's a good hash
        
        $hashParts = preg_split( "/_/", $password_hash );

        // default, to show in log message on failure
        // gets replaced if cookie contains a good hash
        $password = "(bad cookie:  $password_hash)";

        $badCookie = true;
        
        if( count( $hashParts ) == 2 ) {
            
            $salt = $hashParts[0];
            $hash = $hashParts[1];

            foreach( $accessPasswords as $truePassword ) {    
                $trueHash = md5( $salt . $truePassword );
            
                if( $trueHash == $hash ) {
                    $password = $truePassword;
                    $badCookie = false;
                    }
                }
            
            }
        }
    else {
        // no request variable, no cookie
        // cookie probably expired
        $badCookie = true;
        $password_hash = "(no cookie.  expired?)";
        }
    
        
    
    if( ! in_array( $password, $accessPasswords ) ) {

        if( ! $badCookie ) {
            
            echo "Incorrect password.";

            ag_log( "Failed $inFunctionName access with password:  ".
                    "$password" );
            }
        else {
            echo "Session expired.";
                
            ag_log( "Failed $inFunctionName access with bad cookie:  ".
                    "$password_hash" );
            }
        
        die();
        }
    else {
        
        if( $passwordSent && $enableYubikey ) {
            global $yubikeyIDs, $yubicoClientID, $yubicoSecretKey,
                $passwordHashingPepper;
            
            $yubikey = $_REQUEST[ "yubikey" ];

            $index = array_search( $password, $accessPasswords );
            $yubikeyIDList = preg_split( "/:/", $yubikeyIDs[ $index ] );

            $providedID = substr( $yubikey, 0, 12 );

            if( ! in_array( $providedID, $yubikeyIDList ) ) {
                echo "Provided Yubikey does not match ID for this password.";
                die();
                }
            
            
            $nonce = ag_hmac_sha1( $passwordHashingPepper, uniqid() );
            
            $callURL =
                "https://api2.yubico.com/wsapi/2.0/verify?id=$yubicoClientID".
                "&otp=$yubikey&nonce=$nonce";
            
            $result = trim( file_get_contents( $callURL ) );

            $resultLines = preg_split( "/\s+/", $result );

            sort( $resultLines );

            $resultPairs = array();

            $messageToSignParts = array();
            
            foreach( $resultLines as $line ) {
                // careful here, because = is used in base-64 encoding
                // replace first = in a line (the key/value separator)
                // with #
                
                $lineToParse = preg_replace( '/=/', '#', $line, 1 );

                // now split on # instead of =
                $parts = preg_split( "/#/", $lineToParse );

                $resultPairs[$parts[0]] = $parts[1];

                if( $parts[0] != "h" ) {
                    // include all but signature in message to sign
                    $messageToSignParts[] = $line;
                    }
                }
            $messageToSign = implode( "&", $messageToSignParts );

            $trueSig =
                base64_encode(
                    hash_hmac( 'sha1',
                               $messageToSign,
                               // need to pass in raw key
                               base64_decode( $yubicoSecretKey ),
                               true) );
            
            if( $trueSig != $resultPairs["h"] ) {
                echo "Yubikey authentication failed.<br>";
                echo "Bad signature from authentication server<br>";
                die();
                }

            $status = $resultPairs["status"];
            if( $status != "OK" ) {
                echo "Yubikey authentication failed: $status";
                die();
                }

            }
        
        // set cookie again, renewing it, expires in 24 hours
        $expireTime = time() + 60 * 60 * 24;
    
        setcookie( $cookieName, $password_hash, $expireTime, "/" );
        }
    }
 



function ag_clearPasswordCookie() {
    global $tableNamePrefix;

    $cookieName = $tableNamePrefix . "cookie_password_hash";

    // expire 24 hours ago (to avoid timezone issues)
    $expireTime = time() - 60 * 60 * 24;

    setcookie( $cookieName, "", $expireTime, "/" );
    }
 
 







function ag_hmac_sha1( $inKey, $inData ) {
    return hash_hmac( "sha1", 
                      $inData, $inKey );
    } 

 
function ag_hmac_sha1_raw( $inKey, $inData ) {
    return hash_hmac( "sha1", 
                      $inData, $inKey, true );
    } 


 
 
 
 
// decodes a ASCII hex string into an array of 0s and 1s 
function ag_hexDecodeToBitString( $inHexString ) {
    $digits = str_split( $inHexString );

    $bitString = "";

    foreach( $digits as $digit ) {
        $index = hexdec( $digit );

        $binDigitString = decbin( $index );

        // pad with 0s
        $binDigitString =
            substr( "0000", 0, 4 - strlen( $binDigitString ) ) .
            $binDigitString;

        $bitString = $bitString . $binDigitString;
        }

    return $bitString;
    }
 


 
?>
