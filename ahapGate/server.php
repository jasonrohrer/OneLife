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
else if( $action == "register_vote" ) {
    ag_registerVote();
    }
else if( $action == "register_github" ) {
    ag_registerGithub();
    }
else if( $action == "get_content_leader" ) {
    ag_getContentLeader();
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
else if( $action == "logout" ) {
    ag_logout();
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
        ag_doesTableExist( $tableNamePrefix . "log" );
    
        
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
            "github_email VARCHAR(254) NOT NULL," .
            "content_leader_email_vote VARCHAR(254) NOT NULL," .
            "grant_time DATETIME NOT NULL, ".
            "last_vote_time DATETIME NOT NULL );";

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
    

    echo "<table width='100%' border=0><tr>".
        "<td>[<a href=\"server.php?action=show_data" .
            "\">Main</a>]</td>".
        "<td align=right>[<a href=\"server.php?action=logout" .
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
            "OR id LIKE '%$search%' ) ";

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
             <td align=right>
             <FORM ACTION="server.php" METHOD="post">
    <INPUT TYPE="hidden" NAME="action" VALUE="restore_all_tokens">
    <INPUT TYPE="Submit" VALUE="Restore All Tokens">
    <INPUT TYPE="checkbox" NAME="confirm" VALUE=1> Confirm      
    
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
    echo "<td>".orderLink( "github_email", "Github Email" )."</td>\n";
    echo "<td>".orderLink( "content_leader_email_vote",
                           "Chosen Leader" )."</td>\n";
    echo "<td>".orderLink( "grant_time", "Grant Time" )."</td>\n";
    echo "<td>".orderLink( "last_vote_time", "Last Vote Time" )."</td>\n";
    echo "</tr>\n";


    for( $i=0; $i<$numRows; $i++ ) {
        $id = ag_mysqli_result( $result, $i, "id" );
        $email = ag_mysqli_result( $result, $i, "email" );
        $github_email = ag_mysqli_result( $result, $i, "github_email" );
        $content_leader_email_vote =
            ag_mysqli_result( $result, $i, "content_leader_email_vote" );
        $grant_time = ag_mysqli_result( $result, $i, "grant_time" );
        $last_vote_time = ag_mysqli_result( $result, $i, "last_vote_time" );

        
        $encodedEmail = urlencode( $email );

        
        echo "<tr>\n";

        echo "<td>$id</td>\n";
        echo "<td>".
            "<a href=\"server.php?action=show_detail&email=$encodedEmail\">".
            "$email</a></td>\n";
        echo "<td>$github_email</td>\n";
        echo "<td>$content_leader_email_vote</td>\n";
        echo "<td>$grant_time</td>\n";
        echo "<td>$last_vote_time</td>\n";
        echo "</tr>\n";
        }
    echo "</table>";


    echo "<hr>";
    
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
            
    $query = "SELECT id, github_email, content_leader_email_vote ".
        "FROM $tableNamePrefix"."users ".
        "WHERE email = '$email';";
    $result = ag_queryDatabase( $query );

    $id = ag_mysqli_result( $result, 0, "id" );
    $github_email = ag_mysqli_result( $result, 0, "github_email" );
    $content_leader_email_vote =
        ag_mysqli_result( $result, 0, "content_leader_email_vote" );
    
    echo "<center><table border=0><tr><td>";
    
    echo "<b>ID:</b> $id<br><br>";
    echo "<b>Email:</b> $email<br><br>";
    echo "<b>Github Email:</b> $github_email<br><br>";
    echo "<b>Content Leader:</b> $content_leader_email_vote<br><br>";
    echo "<br><br>";
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
        ag_log( "$action denied for stale sequence number" );

        echo "DENIED";
        die();
        }

    $computedHashValue =
        strtoupper( ag_hmac_sha1( $sharedGameServerSecret,
                                  $sequence_number . $concatData ) );

    if( $computedHashValue != $hash_value ) {
        ag_log( "$action denied for bad hash value (correct = $computedHashValue )" );

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

    $result = file_get_contents(
        'https://partner.steam-api.com/ISteamUser/GrantPackage/v1',
        false, $context );

    // not sure about format of results from GrantPackage

    
    // instead, just turn around immediately and check for ownership
    $ownsAppNow = ag_doesSteamUserOwnApp( $inSteamID );

    if( $ownsAppNow == 0 ) {
        // granting failed
        echo "Unlocking attempt response:<br>".
            "<pre>$result</pre><br><pre>";
        echo $http_response_header[0];
        echo "</pre><br>";

        $header = $http_response_header[0];
        ag_log( "GrantPackage failed.  ".
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






function ag_grant() {
    global $tableNamePrefix;

    $email = ag_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i", "" );


    // concat email with sequence number, to prevent replaying grant
    // action with seq of 0 for different emails
    $trueSeq = ag_checkSeqHash( $email, $email );
    
    
    if( $trueSeq == 0 ) {
        // no record exists, grant can go through


        
        // check of OHOL ticket exists
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
            echo "DENIED";
            return;
            }
        

        
        // create AHAP ticket
        global $tableNamePrefixAHAPTicketServer;
        
        $query = "INSERT INTO $tableNamePrefixAHAPTicketServer". "tickets ".
            "VALUES ( " .
            "'$ticket_id', CURRENT_TIMESTAMP, CURRENT_TIMESTAMP, ".
            "'$name', '$email', '$order_number', ".
            "'$tag', '', '0', '0', '0', " .
            "'$email_opt_in' );";

        ag_queryDatabase( $query );

        
        
        if( preg_match( "/@steamgames\.com/", $email ) ) {
            $steamID = preg_replace( '/@steamgames\.com/', '', $email, 1 );

            // grant package on Steam

            $ownerID = ag_grantPackage( $steamID );


            if( $ownerID == 0 ) {
                // failed to grant on Steam

                // clean up AHAP ticket server entry that
                // we created
                $query = "DELETE FROM $tableNamePrefixAHAPTicketServer".
                    "tickets WHERE email = '$email';";
                ag_queryDatabase( $query );

                echo "DENIED";
                return;
                }
            }
        


        

        
        // finally, create a record for this user here
        $query = "INSERT INTO $tableNamePrefix". "users SET " .
            "email = '$email', ".
            "github_email = '$email', ".
            "content_leader_email_vote = '', ".
            "sequence_number = 1, ".
            "grant_time = CURRENT_TIMESTAMP ".
            "last_vote_time = CURRENT_TIMESTAMP ".
            "ON DUPLICATE KEY UPDATE sequence_number = sequence_number + 1 ;";
        ag_queryDatabase( $query );

        echo "OK";
        return;
        }
    else {
        // record already exists for this email, can't re-grant
        echo "DENIED";
        return;
        }
    }




function ag_registerVote() {
    global $tableNamePrefix;

    $email = ag_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i", "" );
    $leader_email = ag_requestFilter( "leader_email",
                                      "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i", "" );

    // include both emails in hash check, so we can't replay voting
    // for a given leader, with a given sequence number for a different email
    $trueSeq = ag_checkSeqHash( $email, $email . $leader_email );
    
    
    if( $trueSeq == 0 ) {

        // no record of this player, can't vote
        echo "DENIED";
        return;
        }
    
    // update the existing one
    $query = "UPDATE $tableNamePrefix"."users SET " .
        "sequence_number = sequence_number + 1, ".
        "content_leader_email_vote = '$leader_email', ".
        "last_vote_time = CURRENT_TIMESTAMP ".
        "WHERE email = '$email'; ";
    
    ag_queryDatabase( $query );
    
    echo "OK";
    }




// only works for emails that have records already, as created by the
// grant action
//
// returns valid email, on success
// prints DENIED and dies on failure
function ag_checkTicketServerSeqHash() {
    global $action;

    $email = ag_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i", "" );
    
    if( $email == "" ) {
        ag_log( "$action token denied for bad email" );
        
        echo "DENIED";
        die();
        }
    
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

    
    $result = trim( file_get_contents(
                        "$ahapTicketServerURL".
                        "?action=check_ticket_hash".
                        "&email=$encodedEmail".
                        "&hash_value=$hash_value".
                        "&string_to_hash=$sequence_number" ) );

    if( $result != "VALID" ) {
        ag_log( "$action denied for bad hash value" );

        echo "DENIED";
        die();
        }

    return $email;
    }

    


function ag_registerGithub() {
    global $tableNamePrefix;

    $github_email = ag_requestFilter( "github_email",
                                      "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i", "" );

    // will die on failure
    $email = ag_checkTicketServerSeqHash();
    
    
    $query = "UPDATE $tableNamePrefix"."users SET " .
        "sequence_number = sequence_number + 1, ".
        "github_email = '$github_email' ".
        "WHERE email = '$email'; ";
    
    ag_queryDatabase( $query );
    
    echo "OK";
    }



function ag_getContentLeader() {
    global $tableNamePrefix;

    // will die on failure
    $email = ag_checkTicketServerSeqHash();


    // got here, hash check succeeded  

    $query = "SELECT content_leader_email " .
        "FROM$tableNamePrefix"."server_globals ;";

    $result = ag_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );

    if( $numRows < 1 ) {
        echo "DENIED";
        return;
        }
    $leader = ag_mysqli_result( $result, 0, "content_leader_email" );

    // update sequence number
    $query = "UPDATE $tableNamePrefix"."users SET " .
        "sequence_number = sequence_number + 1 ".
        "WHERE email = '$email'; ";
    
    ag_queryDatabase( $query );
    
    echo "$leader\n";
    echo "OK";
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
