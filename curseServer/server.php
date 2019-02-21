<?php



global $cs_version;
$cs_version = "1";



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
<HEAD><TITLE>Curse Server Web-based setup</TITLE></HEAD>
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
    $_GET     = array_map( 'cs_stripslashes_deep', $_GET );
    $_POST    = array_map( 'cs_stripslashes_deep', $_POST );
    $_REQUEST = array_map( 'cs_stripslashes_deep', $_REQUEST );
    $_COOKIE  = array_map( 'cs_stripslashes_deep', $_COOKIE );
    }
    


// Check that the referrer header is this page, or kill the connection.
// Used to block XSRF attacks on state-changing functions.
// (To prevent it from being dangerous to surf other sites while you are
// logged in as admin.)
// Thanks Chris Cowan.
function cs_checkReferrer() {
    global $fullServerURL;
    
    if( !isset($_SERVER['HTTP_REFERER']) ||
        strpos($_SERVER['HTTP_REFERER'], $fullServerURL) !== 0 ) {
        
        die( "Bad referrer header" );
        }
    }




// all calls need to connect to DB, so do it once here
cs_connectToDatabase();

// close connection down below (before function declarations)


// testing:
//sleep( 5 );


// general processing whenver server.php is accessed directly




// grab POST/GET variables
$action = cs_requestFilter( "action", "/[A-Z_]+/i" );

$debug = cs_requestFilter( "debug", "/[01]/" );

$remoteIP = "";
if( isset( $_SERVER[ "REMOTE_ADDR" ] ) ) {
    $remoteIP = $_SERVER[ "REMOTE_ADDR" ];
    }




if( $action == "version" ) {
    global $cs_version;
    echo "$cs_version";
    }
else if( $action == "get_sequence_number" ) {
    cs_getSequenceNumber();
    }
else if( $action == "curse" ) {
    cs_curse();
    }
else if( $action == "live_time" ) {
    cs_liveTime();
    }
else if( $action == "is_cursed" ) {
    cs_isCursed();
    }
else if( $action == "show_log" ) {
    cs_showLog();
    }
else if( $action == "clear_log" ) {
    cs_clearLog();
    }
else if( $action == "show_data" ) {
    cs_showData();
    }
else if( $action == "show_detail" ) {
    cs_showDetail();
    }
else if( $action == "logout" ) {
    cs_logout();
    }
else if( $action == "cs_setup" ) {
    global $setup_header, $setup_footer;
    echo $setup_header; 

    echo "<H2>Curse Server Web-based Setup</H2>";

    echo "Creating tables:<BR>";

    echo "<CENTER><TABLE BORDER=0 CELLSPACING=0 CELLPADDING=1>
          <TR><TD BGCOLOR=#000000>
          <TABLE BORDER=0 CELLSPACING=0 CELLPADDING=5>
          <TR><TD BGCOLOR=#FFFFFF>";

    cs_setupDatabase();

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
    $exists = cs_doesTableExist( $tableNamePrefix . "servers" ) &&
        cs_doesTableExist( $tableNamePrefix . "users" ) &&
        cs_doesTableExist( $tableNamePrefix . "lives" ) &&
        cs_doesTableExist( $tableNamePrefix . "log" );
    
        
    if( $exists  ) {
        echo "Curse Server database setup and ready";
        }
    else {
        // start the setup procedure

        global $setup_header, $setup_footer;
        echo $setup_header; 

        echo "<H2>Curse Server Web-based Setup</H2>";
    
        echo "Curse Server will walk you through a " .
            "brief setup process.<BR><BR>";
        
        echo "Step 1: ".
            "<A HREF=\"server.php?action=cs_setup\">".
            "create the database tables</A>";

        echo $setup_footer;
        }
    }



// done processing
// only function declarations below

cs_closeDatabase();







/**
 * Creates the database tables needed by seedBlogs.
 */
function cs_setupDatabase() {
    global $tableNamePrefix;

    $tableName = $tableNamePrefix . "log";
    if( ! cs_doesTableExist( $tableName ) ) {

        // this table contains general info about the server
        // use INNODB engine so table can be locked
        $query =
            "CREATE TABLE $tableName(" .
            "entry TEXT NOT NULL, ".
            "entry_time DATETIME NOT NULL );";

        $result = cs_queryDatabase( $query );

        echo "<B>$tableName</B> table created<BR>";
        }
    else {
        echo "<B>$tableName</B> table already exists<BR>";
        }

    
    
    $tableName = $tableNamePrefix . "users";
    if( ! cs_doesTableExist( $tableName ) ) {

        $query =
            "CREATE TABLE $tableName(" .
            "id INT UNSIGNED NOT NULL PRIMARY KEY AUTO_INCREMENT," .
            "email VARCHAR(254) NOT NULL," .
            "UNIQUE KEY( email )," .
            "sequence_number INT NOT NULL," .
            "curse_score INT NOT NULL," .
            "extra_life_sec FLOAT NOT NULL," .
            "total_curse_score INT NOT NULL );";

        $result = cs_queryDatabase( $query );

        echo "<B>$tableName</B> table created<BR>";
        }
    else {
        echo "<B>$tableName</B> table already exists<BR>";
        }
    }



function cs_showLog() {
    cs_checkPassword( "show_log" );

     echo "[<a href=\"server.php?action=show_data" .
         "\">Main</a>]<br><br><br>";
    
    global $tableNamePrefix;

    $query = "SELECT * FROM $tableNamePrefix"."log ".
        "ORDER BY entry_time DESC;";
    $result = cs_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );



    echo "<a href=\"server.php?action=clear_log\">".
        "Clear log</a>";
        
    echo "<hr>";
        
    echo "$numRows log entries:<br><br><br>\n";
        

    for( $i=0; $i<$numRows; $i++ ) {
        $time = cs_mysqli_result( $result, $i, "entry_time" );
        $entry = htmlspecialchars( cs_mysqli_result( $result, $i, "entry" ) );

        echo "<b>$time</b>:<br>$entry<hr>\n";
        }
    }



function cs_clearLog() {
    cs_checkPassword( "clear_log" );

     echo "[<a href=\"server.php?action=show_data" .
         "\">Main</a>]<br><br><br>";
    
    global $tableNamePrefix;

    $query = "DELETE FROM $tableNamePrefix"."log;";
    $result = cs_queryDatabase( $query );
    
    if( $result ) {
        echo "Log cleared.";
        }
    else {
        echo "DELETE operation failed?";
        }
    }




















function cs_logout() {
    cs_checkReferrer();
    
    cs_clearPasswordCookie();

    echo "Logged out";
    }




function cs_showData( $checkPassword = true ) {
    // these are global so they work in embeded function call below
    global $skip, $search, $order_by;

    if( $checkPassword ) {
        cs_checkPassword( "show_data" );
        }
    
    global $tableNamePrefix, $remoteIP;
    

    echo "<table width='100%' border=0><tr>".
        "<td>[<a href=\"server.php?action=show_data" .
            "\">Main</a>]</td>".
        "<td align=right>[<a href=\"server.php?action=logout" .
            "\">Logout</a>]</td>".
        "</tr></table><br><br><br>";




    $skip = cs_requestFilter( "skip", "/[0-9]+/", 0 );
    
    global $usersPerPage;
    
    $search = cs_requestFilter( "search", "/[A-Z0-9_@. \-]+/i" );

    $order_by = cs_requestFilter( "order_by", "/[A-Z_]+/i",
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

    $result = cs_queryDatabase( $query );
    $totalRecords = cs_mysqli_result( $result, 0, 0 );


    $orderDir = "DESC";

    if( $order_by == "email" ) {
        $orderDir = "ASC";
        }
    
             
    $query = "SELECT * ".
        "FROM $tableNamePrefix"."users $keywordClause".
        "ORDER BY $order_by $orderDir ".
        "LIMIT $skip, $usersPerPage;";
    $result = cs_queryDatabase( $query );
    
    $numRows = mysqli_num_rows( $result );

    $startSkip = $skip + 1;
    
    $endSkip = $startSkip + $usersPerPage - 1;

    if( $endSkip > $totalRecords ) {
        $endSkip = $totalRecords;
        }



        // form for searching users
?>
        <hr>
            <FORM ACTION="server.php" METHOD="post">
    <INPUT TYPE="hidden" NAME="action" VALUE="show_data">
    <INPUT TYPE="hidden" NAME="order_by" VALUE="<?php echo $order_by;?>">
    <INPUT TYPE="text" MAXLENGTH=40 SIZE=20 NAME="search"
             VALUE="<?php echo $search;?>">
    <INPUT TYPE="Submit" VALUE="Search">
    </FORM>
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
    echo "<td>".orderLink( "curse_score", "Current Curse Score" )."</td>\n";
    echo "<td>".orderLink( "total_curse_score", "Total Curse Score" )."</td>\n";
    echo "<td>ExtraSec</td>\n";
    echo "</tr>\n";


    for( $i=0; $i<$numRows; $i++ ) {
        $id = cs_mysqli_result( $result, $i, "id" );
        $email = cs_mysqli_result( $result, $i, "email" );
        $curse_score = cs_mysqli_result( $result, $i, "curse_score" );
        $extra_life_sec = cs_mysqli_result( $result, $i, "extra_life_sec" );
        $total_curse_score =
            cs_mysqli_result( $result, $i, "total_curse_score" );
        
        $encodedEmail = urlencode( $email );

        
        echo "<tr>\n";

        echo "<td>$id</td>\n";
        echo "<td>".
            "<a href=\"server.php?action=show_detail&email=$encodedEmail\">".
            "$email</a></td>\n";
        echo "<td>$curse_score</td>\n";
        echo "<td>$total_curse_score</td>\n";
        echo "<td>$extra_life_sec</td>\n";
        echo "</tr>\n";
        }
    echo "</table>";


    echo "<hr>";
    
    echo "<a href=\"server.php?action=show_log\">".
        "Show log</a>";
    echo "<hr>";
    echo "Generated for $remoteIP\n";

    }



function cs_showDetail( $checkPassword = true ) {
    if( $checkPassword ) {
        cs_checkPassword( "show_detail" );
        }
    
    echo "[<a href=\"server.php?action=show_data" .
         "\">Main</a>]<br><br><br>";
    
    global $tableNamePrefix;
    

    $email = cs_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i" );
            
    $query = "SELECT id, curse_score, total_curse_score, extra_life_sec ".
        "FROM $tableNamePrefix"."users ".
        "WHERE email = '$email';";
    $result = cs_queryDatabase( $query );

    $id = cs_mysqli_result( $result, 0, "id" );
    $curse_score = cs_mysqli_result( $result, 0, "curse_score" );
    $total_curse_score = cs_mysqli_result( $result, 0, "total_curse_score" );
    $extra_life_sec = cs_mysqli_result( $result, 0, "extra_life_sec" );

    

    echo "<center><table border=0><tr><td>";
    
    echo "<b>ID:</b> $id<br><br>";
    echo "<b>Email:</b> $email<br><br>";
    echo "<b>Current Curse Score:</b> $curse_score<br><br>";
    echo "<b>Total Curse Score:</b> $total_curse_score<br><br>";
    echo "<b>ExtraLifeSec:</b> $extra_life_sec<br><br>";
    echo "<br><br>";
    }







function cs_getSequenceNumber() {
    global $tableNamePrefix;
    

    $email = cs_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i", "" );

    if( $email == "" ) {
        cs_log( "getSequenceNumber denied for bad email" );

        echo "DENIED";
        return;
        }
    
    
    $seq = cs_getSequenceNumberForEmail( $email );

    echo "$seq\n"."OK";
    }



// assumes already-filtered, valid email
// returns 0 if not found
function cs_getSequenceNumberForEmail( $inEmail ) {
    global $tableNamePrefix;
    
    $query = "SELECT sequence_number FROM $tableNamePrefix"."users ".
        "WHERE email = '$inEmail';";
    $result = cs_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );

    if( $numRows < 1 ) {
        return 0;
        }
    else {
        return cs_mysqli_result( $result, 0, "sequence_number" );
        }
    }



function cs_scaleCurseScore( $inEmail ) {
    global $tableNamePrefix, $secondsPerCurseScoreDecrement;
    
    $query = "SELECT curse_score, total_curse_score, extra_life_sec ".
        "FROM $tableNamePrefix"."users ".
        "WHERE email = '$inEmail';"; 

    $result = cs_queryDatabase( $query );
        
    $numRows = mysqli_num_rows( $result );

    if( $numRows == 1 ) {
        $curse_score = cs_mysqli_result( $result, 0, "curse_score" );
        $total_curse_score =
            cs_mysqli_result( $result, 0, "total_curse_score" );
        $extra_life_sec = cs_mysqli_result( $result, 0, "extra_life_sec" );
        
        global $curseThreshold, $lifetimeThresholds, $hoursToServe,
            $servingThreshold;

        $personalThreshold = $curseThreshold;

        $index = 0;
        foreach( $lifetimeThresholds as $thresh ) {
            if( $total_curse_score >= $thresh ) {
                $personalThreshold = $servingThreshold[ $index ];
                }
            $index++;
            }

        
        if( $curse_score >= $personalThreshold ) {

            $hours = 0;

            $index = 0;
            foreach( $lifetimeThresholds as $thresh ) {
                if( $total_curse_score >= $thresh ) {
                    $hours = $hoursToServe[ $index ];
                    }
                $index++;
                }

            if( $hours > 0 && $hours < 1 ) {
                $extra_life_sec += $hours * $secondsPerCurseScoreDecrement;

                $hours = 1;
                }
            $hours = floor( $hours );
            
            // score encodes hours to serve
            // one hour puts them right at the threshold
            $curse_score = $personalThreshold + $hours - 1;

            $query = "UPDATE $tableNamePrefix"."users SET " .
                "curse_score = $curse_score, " .
                "extra_life_sec = $extra_life_sec " .
                "WHERE email = '$inEmail'; ";
            
            $result = cs_queryDatabase( $query );
            }
        }
    }





function cs_curse() {
    global $tableNamePrefix, $sharedGameServerSecret;

    // no locking is done here, because action is asynchronous anyway
    // and there's no way to prevent a server from acting on a stale
    // sequence number if calls for the same email are interleaved

    // however, we only want to support a given email address playing on
    // one server at a time, so it's okay if some parallel lives are
    // not logged correctly
    

    $email = cs_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i", "" );

    $sequence_number = cs_requestFilter( "sequence_number", "/[0-9]+/i", "0" );

    $hash_value = cs_requestFilter( "hash_value", "/[A-F0-9]+/i", "" );

    $hash_value = strtoupper( $hash_value );


    if( $email == "" ) {

        cs_log( "curse denied for bad email or server name" );
        
        echo "DENIED";
        return;
        }
    
    $trueSeq = cs_getSequenceNumberForEmail( $email );

    if( $trueSeq > $sequence_number ) {
        cs_log( "curse denied for stale sequence number" );

        echo "DENIED";
        return;
        }

    $computedHashValue =
        strtoupper( cs_hmac_sha1( $sharedGameServerSecret, $sequence_number ) );

    if( $computedHashValue != $hash_value ) {
        cs_log( "curse denied for bad hash value" );

        echo "DENIED";
        return;
        }

    if( $trueSeq == 0 ) {
        // no record exists, add one
        $query = "INSERT INTO $tableNamePrefix". "users SET " .
            "email = '$email', ".
            "sequence_number = 1, ".
            "curse_score = 1, ".
            "total_curse_score = 1, ".
            "extra_life_sec = 0 ".
            "ON DUPLICATE KEY UPDATE sequence_number = sequence_number + 1, ".
            "extra_life_sec = 0, ".
            "curse_score = curse_score + 1, ".
            "total_curse_score = total_curse_score + 1;";
        }
    else {
        // update the existing one
        $query = "UPDATE $tableNamePrefix"."users SET " .
            // our values might be stale, increment values in table
            "sequence_number = sequence_number + 1, ".
            // reset their accumulated lifetime counter when they
            // get additional curse points.
            "extra_life_sec = 0, ".
            "curse_score = curse_score + 1, " .
            "total_curse_score = total_curse_score + 1 " .
            "WHERE email = '$email'; ";
        
        }

    cs_queryDatabase( $query );
    
    cs_scaleCurseScore( $email );    
    
    echo "OK";
    }




function cs_liveTime() {
    global $tableNamePrefix, $sharedGameServerSecret,
        $secondsPerCurseScoreDecrement;

    // no locking is done here, because action is asynchronous anyway
    // and there's no way to prevent a server from acting on a stale
    // sequence number if calls for the same email are interleaved

    // however, we only want to support a given email address playing on
    // one server at a time, so it's okay if some parallel lives are
    // not logged correctly
    

    $email = cs_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i", "" );

    $seconds = cs_requestFilter( "seconds", "/[0-9.]+/i", "0" );
    
    $sequence_number = cs_requestFilter( "sequence_number", "/[0-9]+/i", "0" );

    $hash_value = cs_requestFilter( "hash_value", "/[A-F0-9]+/i", "" );

    $hash_value = strtoupper( $hash_value );


    if( $email == "" ) {

        cs_log( "live_time denied for bad email or server name" );
        
        echo "DENIED";
        return;
        }
    
    $trueSeq = cs_getSequenceNumberForEmail( $email );

    if( $trueSeq > $sequence_number ) {
        cs_log( "live_time denied for stale sequence number" );

        echo "DENIED";
        return;
        }

    $computedHashValue =
        strtoupper( cs_hmac_sha1( $sharedGameServerSecret, $sequence_number ) );

    if( $computedHashValue != $hash_value ) {
        cs_log( "live_time denied for bad hash value" );

        echo "DENIED";
        return;
        }


    global $minServedSecondsCount;
    if( $seconds < $minServedSecondsCount ) {
        // life is too short to count as time served.
        echo "OK";
        return;
        }
    
    
    if( $trueSeq == 0 ) {
        // no record exists, add one
        $query = "INSERT INTO $tableNamePrefix". "users SET " .
            "email = '$email', ".
            "sequence_number = 1, ".
            "curse_score = 0, ".
            "total_curse_score = 0, ".
            // don't count lived time unless already cursed
            "extra_life_sec = 0 ".
            "ON DUPLICATE KEY UPDATE sequence_number = sequence_number + 1;";
        }
    else {
        // update the existing one

        $query = "SELECT curse_score, extra_life_sec ".
            "FROM $tableNamePrefix"."users ".
            "WHERE email = '$email';";
        $result = cs_queryDatabase( $query );

        $numRows = mysqli_num_rows( $result );

        if( $numRows > 0 ) {

            $curse_score = cs_mysqli_result( $result, 0, "curse_score" );
            $extra_life_sec = cs_mysqli_result( $result, 0, "extra_life_sec" );


            if( $curse_score > 0 ) {
                $totalLifeSec = $seconds + $extra_life_sec;

                while( $totalLifeSec >= $secondsPerCurseScoreDecrement &&
                       $curse_score > 0 ) {
                    $curse_score --;
                    $totalLifeSec -= $secondsPerCurseScoreDecrement;
                    }

                if( $curse_score > 0 ) {
                    $extra_life_sec = $totalLifeSec;
                    }
                else {
                    $extra_life_sec = 0;
                    }
                

                // never decrement total_curse_score
                
                $query = "UPDATE $tableNamePrefix"."users SET " .
                    // our values might be stale, increment values in table
                    "sequence_number = sequence_number + 1, ".
                    "curse_score = $curse_score, " .
                    "extra_life_sec = $extra_life_sec " .
                    "WHERE email = '$email'; ";
                }
            }
        }

    cs_queryDatabase( $query );

    echo "OK";
    }




function cs_isCursed() {
    global $tableNamePrefix, $sharedGameServerSecret, $curseThreshold;

    // no locking is done here, because action is asynchronous anyway
    // and there's no way to prevent a server from acting on a stale
    // sequence number if calls for the same email are interleaved

    // however, we only want to support a given email address playing on
    // one server at a time, so it's okay if some parallel lives are
    // not logged correctly
    

    $email = cs_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i", "" );

    $email_hash_value =
        cs_requestFilter( "email_hash_value", "/[A-F0-9]+/i", "" );

    $email_hash_value = strtoupper( $email_hash_value );


    if( $email == "" ) {

        cs_log( "isCursed denied for bad email or server name" );
        
        echo "DENIED";
        return;
        }

    $computedHashValue =
        strtoupper( cs_hmac_sha1( $sharedGameServerSecret, $email ) );

    if( $computedHashValue != $email_hash_value ) {
        cs_log( "isCursed denied for bad hash value" );

        echo "DENIED";
        return;
        }


    $query = "SELECT curse_score, total_curse_score ".
            "FROM $tableNamePrefix"."users ".
            "WHERE email = '$email';";
    $result = cs_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );
    
    if( $numRows = 0 ) {
        // player doesn't exist, they are automatically not cursed
        echo "0 0";
        return;
        }

    $curse_score = cs_mysqli_result( $result, 0, "curse_score" );
    $total_curse_score = cs_mysqli_result( $result, 0, "total_curse_score" );

    global $lifetimeThresholds, $hoursToServe, $servingThreshold;
    
    $personalThreshold = $curseThreshold;

    $index = 0;
    foreach( $lifetimeThresholds as $thresh ) {
        if( $total_curse_score >= $thresh ) {
            $personalThreshold = $servingThreshold[ $index ];
            }
        $index++;
        }

    
    if( $curse_score >= $personalThreshold ) {
        $excess = ( $curse_score - $personalThreshold ) + 1;
        echo "1 $excess";
        return;
        }
    echo "0 0";
    }







$cs_mysqlLink;


// general-purpose functions down here, many copied from seedBlogs

/**
 * Connects to the database according to the database variables.
 */  
function cs_connectToDatabase() {
    global $databaseServer,
        $databaseUsername, $databasePassword, $databaseName,
        $cs_mysqlLink;
    
    
    $cs_mysqlLink =
        mysqli_connect( $databaseServer, $databaseUsername, $databasePassword )
        or cs_operationError( "Could not connect to database server: " .
                              mysqli_error( $cs_mysqlLink ) );
    
    mysqli_select_db( $cs_mysqlLink, $databaseName )
        or cs_operationError( "Could not select $databaseName database: " .
                              mysqli_error( $cs_mysqlLink ) );
    }


 
/**
 * Closes the database connection.
 */
function cs_closeDatabase() {
    global $cs_mysqlLink;
    
    mysqli_close( $cs_mysqlLink );
    }


/**
 * Returns human-readable summary of a timespan.
 * Examples:  10.5 hours
 *            34 minutes
 *            45 seconds
 */
function cs_secondsToTimeSummary( $inSeconds ) {
    if( $inSeconds < 120 ) {
        if( $inSeconds == 1 ) {
            return "$inSeconds second";
            }
        return "$inSeconds seconds";
        }
    else if( $inSeconds < 3600 ) {
        $min = number_format( $inSeconds / 60, 0 );
        return "$min minutes";
        }
    else {
        $hours = number_format( $inSeconds / 3600, 1 );
        return "$hours hours";
        }
    }


/**
 * Returns human-readable summary of a distance back in time.
 * Examples:  10 hours
 *            34 minutes
 *            45 seconds
 *            19 days
 *            3 months
 *            2.5 years
 */
function cs_secondsToAgeSummary( $inSeconds ) {
    if( $inSeconds < 120 ) {
        if( $inSeconds == 1 ) {
            return "$inSeconds second";
            }
        return "$inSeconds seconds";
        }
    else if( $inSeconds < 3600 * 2 ) {
        $min = number_format( $inSeconds / 60, 0 );
        return "$min minutes";
        }
    else if( $inSeconds < 24 * 3600 * 2 ) {
        $hours = number_format( $inSeconds / 3600, 0 );
        return "$hours hours";
        }
    else if( $inSeconds < 24 * 3600 * 60 ) {
        $days = number_format( $inSeconds / ( 3600 * 24 ), 0 );
        return "$days days";
        }
    else if( $inSeconds < 24 * 3600 * 365 * 2 ) {
        // average number of days per month
        // based on 400 year calendar cycle
        // we skip a leap year every 100 years unless the year is divisible by 4
        $months = number_format( $inSeconds / ( 3600 * 24 * 30.436875 ), 0 );
        return "$months months";
        }
    else {
        // same logic behind computing average length of a year
        $years = number_format( $inSeconds / ( 3600 * 24 * 365.2425 ), 1 );
        return "$years years";
        }
    }



/**
 * Queries the database, and dies with an error message on failure.
 *
 * @param $inQueryString the SQL query string.
 *
 * @return a result handle that can be passed to other mysql functions.
 */
function cs_queryDatabase( $inQueryString ) {
    global $cs_mysqlLink;
    
    if( gettype( $cs_mysqlLink ) != "resource" ) {
        // not a valid mysql link?
        cs_connectToDatabase();
        }
    
    $result = mysqli_query( $cs_mysqlLink, $inQueryString );
    
    if( $result == FALSE ) {

        $errorNumber = mysqli_errno( $cs_mysqlLink );
        
        // server lost or gone?
        if( $errorNumber == 2006 ||
            $errorNumber == 2013 ||
            // access denied?
            $errorNumber == 1044 ||
            $errorNumber == 1045 ||
            // no db selected?
            $errorNumber == 1046 ) {

            // connect again?
            cs_closeDatabase();
            cs_connectToDatabase();

            $result = mysqli_query( $cs_mysqlLink, $inQueryString )
                or cs_operationError(
                    "Database query failed:<BR>$inQueryString<BR><BR>" .
                    mysqli_error( $cs_mysqlLink ) );
            }
        else {
            // some other error (we're still connected, so we can
            // add log messages to database
            cs_fatalError( "Database query failed:<BR>$inQueryString<BR><BR>" .
                           mysqli_error( $cs_mysqlLink ) );
            }
        }

    return $result;
    }



/**
 * Replacement for the old mysql_result function.
 */
function cs_mysqli_result( $result, $number, $field=0 ) {
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
function cs_doesTableExist( $inTableName ) {
    // check if our table exists
    $tableExists = 0;
    
    $query = "SHOW TABLES";
    $result = cs_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );


    for( $i=0; $i<$numRows && ! $tableExists; $i++ ) {

        $tableName = cs_mysqli_result( $result, $i, 0 );
        
        if( $tableName == $inTableName ) {
            $tableExists = 1;
            }
        }
    return $tableExists;
    }



function cs_log( $message ) {
    global $enableLog, $tableNamePrefix, $cs_mysqlLink;

    if( $enableLog ) {
        $slashedMessage = mysqli_real_escape_string( $cs_mysqlLink, $message );
    
        $query = "INSERT INTO $tableNamePrefix"."log VALUES ( " .
            "'$slashedMessage', CURRENT_TIMESTAMP );";
        $result = cs_queryDatabase( $query );
        }
    }



/**
 * Displays the error page and dies.
 *
 * @param $message the error message to display on the error page.
 */
function cs_fatalError( $message ) {
    //global $errorMessage;

    // set the variable that is displayed inside error.php
    //$errorMessage = $message;
    
    //include_once( "error.php" );

    // for now, just print error message
    $logMessage = "Fatal error:  $message";
    
    echo( $logMessage );

    cs_log( $logMessage );
    
    die();
    }



/**
 * Displays the operation error message and dies.
 *
 * @param $message the error message to display.
 */
function cs_operationError( $message ) {
    
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
function cs_addslashes_deep( $inValue ) {
    return
        ( is_array( $inValue )
          ? array_map( 'cs_addslashes_deep', $inValue )
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
function cs_stripslashes_deep( $inValue ) {
    return
        ( is_array( $inValue )
          ? array_map( 'cs_stripslashes_deep', $inValue )
          : stripslashes( $inValue ) );
    }



/**
 * Filters a $_REQUEST variable using a regex match.
 *
 * Returns "" (or specified default value) if there is no match.
 */
function cs_requestFilter( $inRequestVariable, $inRegex, $inDefault = "" ) {
    if( ! isset( $_REQUEST[ $inRequestVariable ] ) ) {
        return $inDefault;
        }

    return cs_filter( $_REQUEST[ $inRequestVariable ], $inRegex, $inDefault );
    }


/**
 * Filters a value  using a regex match.
 *
 * Returns "" (or specified default value) if there is no match.
 */
function cs_filter( $inValue, $inRegex, $inDefault = "" ) {
    
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
function cs_checkPassword( $inFunctionName ) {
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
        $password = cs_hmac_sha1( $passwordHashingPepper,
                                  $_REQUEST[ "passwordHMAC" ] );
        
        
        // generate a new hash cookie from this password
        $newSalt = time();
        $newHash = md5( $newSalt . $password );
        
        $password_hash = $newSalt . "_" . $newHash;
        }
    else if( isset( $_COOKIE[ $cookieName ] ) ) {
        cs_checkReferrer();
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

            cs_log( "Failed $inFunctionName access with password:  ".
                    "$password" );
            }
        else {
            echo "Session expired.";
                
            cs_log( "Failed $inFunctionName access with bad cookie:  ".
                    "$password_hash" );
            }
        
        die();
        }
    else {
        
        if( $passwordSent && $enableYubikey ) {
            global $yubikeyIDs, $yubicoClientID, $yubicoSecretKey,
                $ticketGenerationSecret;
            
            $yubikey = $_REQUEST[ "yubikey" ];

            $index = array_search( $password, $accessPasswords );
            $yubikeyIDList = preg_split( "/:/", $yubikeyIDs[ $index ] );

            $providedID = substr( $yubikey, 0, 12 );

            if( ! in_array( $providedID, $yubikeyIDList ) ) {
                echo "Provided Yubikey does not match ID for this password.";
                die();
                }
            
            
            $nonce = cs_hmac_sha1( $ticketGenerationSecret, uniqid() );
            
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
 



function cs_clearPasswordCookie() {
    global $tableNamePrefix;

    $cookieName = $tableNamePrefix . "cookie_password_hash";

    // expire 24 hours ago (to avoid timezone issues)
    $expireTime = time() - 60 * 60 * 24;

    setcookie( $cookieName, "", $expireTime, "/" );
    }
 
 







function cs_hmac_sha1( $inKey, $inData ) {
    return hash_hmac( "sha1", 
                      $inData, $inKey );
    } 

 
function cs_hmac_sha1_raw( $inKey, $inData ) {
    return hash_hmac( "sha1", 
                      $inData, $inKey, true );
    } 


 
 
 
 
// decodes a ASCII hex string into an array of 0s and 1s 
function cs_hexDecodeToBitString( $inHexString ) {
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
