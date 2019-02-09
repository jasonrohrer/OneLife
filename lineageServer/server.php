<?php



global $ls_version;
$ls_version = "1";



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
<HEAD><TITLE>Lineage Server Web-based setup</TITLE></HEAD>
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
    $_GET     = array_map( 'ls_stripslashes_deep', $_GET );
    $_POST    = array_map( 'ls_stripslashes_deep', $_POST );
    $_REQUEST = array_map( 'ls_stripslashes_deep', $_REQUEST );
    $_COOKIE  = array_map( 'ls_stripslashes_deep', $_COOKIE );
    }
    


// Check that the referrer header is this page, or kill the connection.
// Used to block XSRF attacks on state-changing functions.
// (To prevent it from being dangerous to surf other sites while you are
// logged in as admin.)
// Thanks Chris Cowan.
function ls_checkReferrer() {
    global $fullServerURL;
    
    if( !isset($_SERVER['HTTP_REFERER']) ||
        strpos($_SERVER['HTTP_REFERER'], $fullServerURL) !== 0 ) {
        
        die( "Bad referrer header" );
        }
    }




// all calls need to connect to DB, so do it once here
ls_connectToDatabase();

// close connection down below (before function declarations)


// testing:
//sleep( 5 );


// general processing whenver server.php is accessed directly




// grab POST/GET variables
$action = ls_requestFilter( "action", "/[A-Z_]+/i" );

$debug = ls_requestFilter( "debug", "/[01]/" );

$remoteIP = "";
if( isset( $_SERVER[ "REMOTE_ADDR" ] ) ) {
    $remoteIP = $_SERVER[ "REMOTE_ADDR" ];
    }




if( $action == "version" ) {
    global $ls_version;
    echo "$ls_version";
    }
else if( $action == "get_sequence_number" ) {
    ls_getSequenceNumber();
    }
else if( $action == "log_life" ) {
    ls_logLife();
    }
else if( $action == "show_log" ) {
    ls_showLog();
    }
else if( $action == "clear_log" ) {
    ls_clearLog();
    }
else if( $action == "show_data" ) {
    ls_showData();
    }
else if( $action == "show_detail" ) {
    ls_showDetail();
    }
else if( $action == "logout" ) {
    ls_logout();
    }
else if( $action == "front_page" ) {
    ls_frontPage();
    }
else if( $action == "character_page" ) {
    ls_characterPage();
    }
else if( $action == "character_dump" ) {
    ls_characterDump();
    }
// disable this one for now, no longer needed
else if( false && $action == "reformat_names" ) {
    ls_reformatNames();
    }
else if( $action == "ls_setup" ) {
    global $setup_header, $setup_footer;
    echo $setup_header; 

    echo "<H2>Lineage Server Web-based Setup</H2>";

    echo "Creating tables:<BR>";

    echo "<CENTER><TABLE BORDER=0 CELLSPACING=0 CELLPADDING=1>
          <TR><TD BGCOLOR=#000000>
          <TABLE BORDER=0 CELLSPACING=0 CELLPADDING=5>
          <TR><TD BGCOLOR=#FFFFFF>";

    ls_setupDatabase();

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
    $exists = ls_doesTableExist( $tableNamePrefix . "servers" ) &&
        ls_doesTableExist( $tableNamePrefix . "users" ) &&
        ls_doesTableExist( $tableNamePrefix . "lives" ) &&
        ls_doesTableExist( $tableNamePrefix . "log" );
    
        
    if( $exists  ) {
        echo "Lineage Server database setup and ready";
        }
    else {
        // start the setup procedure

        global $setup_header, $setup_footer;
        echo $setup_header; 

        echo "<H2>Lineage Server Web-based Setup</H2>";
    
        echo "Lineage Server will walk you through a " .
            "brief setup process.<BR><BR>";
        
        echo "Step 1: ".
            "<A HREF=\"server.php?action=ls_setup\">".
            "create the database tables</A>";

        echo $setup_footer;
        }
    }



// done processing
// only function declarations below

ls_closeDatabase();







/**
 * Creates the database tables needed by seedBlogs.
 */
function ls_setupDatabase() {
    global $tableNamePrefix;

    $tableName = $tableNamePrefix . "log";
    if( ! ls_doesTableExist( $tableName ) ) {

        // this table contains general info about the server
        // use INNODB engine so table can be locked
        $query =
            "CREATE TABLE $tableName(" .
            "entry TEXT NOT NULL, ".
            "entry_time DATETIME NOT NULL, ".
            "index( entry_time ) );";

        $result = ls_queryDatabase( $query );

        echo "<B>$tableName</B> table created<BR>";
        }
    else {
        echo "<B>$tableName</B> table already exists<BR>";
        }

    
    
    $tableName = $tableNamePrefix . "servers";
    if( ! ls_doesTableExist( $tableName ) ) {

        $query =
            "CREATE TABLE $tableName(" .
            "id INT UNSIGNED NOT NULL PRIMARY KEY AUTO_INCREMENT," .
            "server VARCHAR(254) NOT NULL, ".
            "UNIQUE KEY( server ) );";

        $result = ls_queryDatabase( $query );

        echo "<B>$tableName</B> table created<BR>";
        }
    else {
        echo "<B>$tableName</B> table already exists<BR>";
        }

    

    $tableName = $tableNamePrefix . "users";
    if( ! ls_doesTableExist( $tableName ) ) {

        $query =
            "CREATE TABLE $tableName(" .
            "id INT UNSIGNED NOT NULL PRIMARY KEY AUTO_INCREMENT," .
            "email VARCHAR(254) NOT NULL," .
            "UNIQUE KEY( email )," .
            "email_sha1 CHAR(40) NOT NULL," .
            "index( email_sha1 ), ".
            "sequence_number INT NOT NULL," .
            "life_count INT NOT NULL );";

        $result = ls_queryDatabase( $query );

        echo "<B>$tableName</B> table created<BR>";
        }
    else {
        echo "<B>$tableName</B> table already exists<BR>";
        }



    $tableName = $tableNamePrefix . "lives";
    if( ! ls_doesTableExist( $tableName ) ) {

        $query =
            "CREATE TABLE $tableName(" .
            "id INT UNSIGNED NOT NULL PRIMARY KEY AUTO_INCREMENT," .
            "death_time DATETIME NOT NULL, ".
            "server_id INT UNSIGNED NOT NULL," .
            "INDEX( server_id )," .
            // ID of player in users table
            "user_id INT UNSIGNED NOT NULL," .
            "INDEX( user_id )," .
            // ID of player on server
            "player_id INT UNSIGNED NOT NULL," .
            "INDEX( player_id )," .
            // ID of parent on server
            // -1 if this player is Eve
            "parent_id INT NOT NULL," .
            "INDEX( parent_id )," .
            // ID of player's killer on server
            // -1 if this player was not murdered
            // -ID if player killed by a non-human object (like a snake)
            "killer_id INT NOT NULL," .
            // string describing death, if not murder
            // Starved
            // Old Age
            // Killed by Snake
            // etc.
            "death_cause VARCHAR(254) NOT NULL,".
            // object ID when player displayed in client
            "display_id INT UNSIGNED NOT NULL," .
            // age at time of death in years
            "age FLOAT UNSIGNED NOT NULL,".
            "name VARCHAR(254) NOT NULL,".
            "INDEX( name ),".
            // 1 if male
            "male TINYINT UNSIGNED NOT NULL,".
            "last_words VARCHAR(63) NOT NULL,".
            // -1 if not set yet
            // 0 for Eve
            "generation INT NOT NULL,".
            // this will make queries ordering by generation and death_time fast
            // OR just queries on generation fast.
            // but not querys on just death_time fast
            "INDEX( generation, death_time ),".
            // single index on death_time to speed up queries on just death_time
            "INDEX( death_time ),".
            // -1 if not set yet
            // 0 for Eve
            // the Eve of this family line
            "eve_life_id INT NOT NULL,".
            // both -1 if not set
            // 0 if set and empty
            "deepest_descendant_generation INT NOT NULL,".
            "deepest_descendant_life_id INT NOT NULL );";

        $result = ls_queryDatabase( $query );

        echo "<B>$tableName</B> table created<BR>";
        }
    else {
        echo "<B>$tableName</B> table already exists<BR>";
        }
    }



function ls_showLog() {
    ls_checkPassword( "show_log" );

    echo "[<a href=\"server.php?action=show_data" .
        "\">Main</a>]<br><br><br>";

    $entriesPerPage = 1000;
    
    $skip = ls_requestFilter( "skip", "/\d+/", 0 );

    
    global $tableNamePrefix;


    // first, count results
    $query = "SELECT COUNT(*) FROM $tableNamePrefix"."log;";

    $result = ls_queryDatabase( $query );
    $totalEntries = ls_mysqli_result( $result, 0, 0 );


    
    $query = "SELECT entry, entry_time FROM $tableNamePrefix"."log ".
        "ORDER BY entry_time DESC LIMIT $skip, $entriesPerPage;";
    $result = ls_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );



    $startSkip = $skip + 1;
    
    $endSkip = $startSkip + $entriesPerPage - 1;

    if( $endSkip > $totalEntries ) {
        $endSkip = $totalEntries;
        }

    

    
    echo "$totalEntries Log entries".
        " (showing $startSkip - $endSkip):<br>\n";

    
    $nextSkip = $skip + $entriesPerPage;

    $prevSkip = $skip - $entriesPerPage;

    if( $skip > 0 && $prevSkip < 0 ) {
        $prevSkip = 0;
        }
    
    if( $prevSkip >= 0 ) {
        echo "[<a href=\"server.php?action=show_log" .
            "&skip=$prevSkip\">".
            "Previous Page</a>] ";
        }
    if( $nextSkip < $totalEntries ) {
        echo "[<a href=\"server.php?action=show_log" .
            "&skip=$nextSkip\">".
            "Next Page</a>]";
        }
    
        
    echo "<hr>";

        
    
    for( $i=0; $i<$numRows; $i++ ) {
        $time = ls_mysqli_result( $result, $i, "entry_time" );
        $entry = htmlspecialchars( ls_mysqli_result( $result, $i, "entry" ) );

        echo "<b>$time</b>:<br><pre>$entry</pre><hr>\n";
        }

    echo "<br><br><hr><a href=\"server.php?action=clear_log\">".
        "Clear log</a>";
    }






function ls_clearLog() {
    ls_checkPassword( "clear_log" );

     echo "[<a href=\"server.php?action=show_data" .
         "\">Main</a>]<br><br><br>";
    
    global $tableNamePrefix;

    $query = "DELETE FROM $tableNamePrefix"."log;";
    $result = ls_queryDatabase( $query );
    
    if( $result ) {
        echo "Log cleared.";
        }
    else {
        echo "DELETE operation failed?";
        }
    }




















function ls_logout() {
    ls_checkReferrer();
    
    ls_clearPasswordCookie();

    echo "Logged out";
    }




function ls_showData( $checkPassword = true ) {
    // these are global so they work in embeded function call below
    global $skip, $search, $order_by;

    if( $checkPassword ) {
        ls_checkPassword( "show_data" );
        }
    
    global $tableNamePrefix, $remoteIP;
    

    echo "<table width='100%' border=0><tr>".
        "<td>[<a href=\"server.php?action=show_data" .
            "\">Main</a>]</td>".
        "<td align=right>[<a href=\"server.php?action=logout" .
            "\">Logout</a>]</td>".
        "</tr></table><br><br><br>";




    $skip = ls_requestFilter( "skip", "/[0-9]+/", 0 );
    
    global $usersPerPage;
    
    $search = ls_requestFilter( "search", "/[A-Z0-9_@. \-]+/i" );

    $order_by = ls_requestFilter( "order_by", "/[A-Z_]+/i",
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

    $result = ls_queryDatabase( $query );
    $totalRecords = ls_mysqli_result( $result, 0, 0 );


    $orderDir = "DESC";

    if( $order_by == "email" ) {
        $orderDir = "ASC";
        }
    
             
    $query = "SELECT * ".
        "FROM $tableNamePrefix"."users $keywordClause".
        "ORDER BY $order_by $orderDir ".
        "LIMIT $skip, $usersPerPage;";
    $result = ls_queryDatabase( $query );
    
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
    echo "<td>".orderLink( "life_count", "LifeCount" )."</td>\n";
    echo "</tr>\n";


    for( $i=0; $i<$numRows; $i++ ) {
        $id = ls_mysqli_result( $result, $i, "id" );
        $email = ls_mysqli_result( $result, $i, "email" );
        $life_count = ls_mysqli_result( $result, $i, "life_count" );

        $encodedEmail = urlencode( $email );

        
        echo "<tr>\n";

        echo "<td>$id</td>\n";
        echo "<td>".
            "<a href=\"server.php?action=show_detail&email=$encodedEmail\">".
            "$email</a></td>\n";
        echo "<td>$life_count</td>\n";
        echo "</tr>\n";
        }
    echo "</table>";


    echo "<hr>";
    
    echo "<a href=\"server.php?action=show_log\">".
        "Show log</a>";
    echo "<hr>";
    echo "Generated for $remoteIP\n";

    }



function ls_showDetail( $checkPassword = true ) {
    if( $checkPassword ) {
        ls_checkPassword( "show_detail" );
        }
    
    echo "[<a href=\"server.php?action=show_data" .
         "\">Main</a>]<br><br><br>";
    
    global $tableNamePrefix;
    

    $email = ls_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i" );
            
    $query = "SELECT id, life_count FROM $tableNamePrefix"."users ".
            "WHERE email = '$email';";
    $result = ls_queryDatabase( $query );

    $id = ls_mysqli_result( $result, 0, "id" );
    $life_count = ls_mysqli_result( $result, 0, "life_count" );

    

    echo "<center><table border=0><tr><td>";
    
    echo "<b>ID:</b> $id<br><br>";
    echo "<b>Email:</b> $email<br><br>";
    echo "<b>Life Count:</b> $life_count<br><br>";
    echo "<br><br>";


    if( $life_count > 0 ) {

        $query = "SELECT id, server_id, death_time, age, name, last_words,".
            "generation ".
            "FROM $tableNamePrefix"."lives ".
            "WHERE user_id = '$id' ORDER BY death_time DESC;";
        $result = ls_queryDatabase( $query );

        $numRows = mysqli_num_rows( $result );

        echo "<table border=1 cellpadding=10>";
        echo "<tr><td><b>Date</b></td>".
            "<td><b>Age</b></td>".
            "<td><b>Name</b></td>".
            "<td><b>Server</b></td>".
            "<td><b>Generation</b></td>".
            "<td><b>Last Words</b></td>".
            "</tr>";
        
        for( $i=0; $i<$numRows; $i++ ) {
            $id = ls_mysqli_result( $result, $i, "id" );
            $death_time = ls_mysqli_result( $result, $i, "death_time" );
            $age = ls_mysqli_result( $result, $i, "age" );
            $name = ls_mysqli_result( $result, $i, "name" );
            $last_words = ls_mysqli_result( $result, $i, "last_words" );
            $server_id = ls_mysqli_result( $result, $i, "server_id" );
            $generation = ls_mysqli_result( $result, $i, "generation" );

            $serverName = ls_getServerName( $server_id );

            if( $generation == -1 ) {
                $generation = ls_getGeneration( $id );
                }
            
            echo "<tr><td>".
                "<a href='server.php?action=character_page&".
                "id=$id'>$death_time</a></td>".
                "<td>$age</td>".
                "<td>$name</td>".
                "<td>$serverName</td>".
                "<td>$generation</td>".
                "<td>$last_words</td>".
                "</tr>";
            }
        
        echo "</table>";
        }
    }







function ls_getSequenceNumber() {
    global $tableNamePrefix;
    

    $email = ls_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i", "" );

    if( $email == "" ) {
        ls_log( "getSequenceNumber denied for bad email" );

        echo "DENIED";
        return;
        }
    
    
    $seq = ls_getSequenceNumberForEmail( $email );

    echo "$seq\n"."OK";
    }



// assumes already-filtered, valid email
// returns 0 if not found
function ls_getSequenceNumberForEmail( $inEmail ) {
    global $tableNamePrefix;
    
    $query = "SELECT sequence_number FROM $tableNamePrefix"."users ".
        "WHERE email = '$inEmail';";
    $result = ls_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );

    if( $numRows < 1 ) {
        return 0;
        }
    else {
        return ls_mysqli_result( $result, 0, "sequence_number" );
        }
    }


// can be called if server doesn't exist yet, and a record is created
function ls_getServerID( $inServer ) {
    global $tableNamePrefix;
    
    $query = "SELECT id FROM $tableNamePrefix"."servers ".
        "WHERE server = '$inServer';";
    $result = ls_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );

    if( $numRows < 1 ) {
        $query = "INSERT INTO $tableNamePrefix". "servers SET " .
            "server = '$inServer' ".
            "ON DUPLICATE KEY UPDATE server = '$inServer' ;";
        ls_queryDatabase( $query );

        // select ID again after insert
        $query = "SELECT id FROM $tableNamePrefix"."servers ".
            "WHERE server = '$inServer';";
        $result = ls_queryDatabase( $query );
        }

    return ls_mysqli_result( $result, 0, "id" );
    }



function ls_getServerName( $inServerID ) {
    global $tableNamePrefix;
    
    $query = "SELECT server FROM $tableNamePrefix"."servers ".
        "WHERE id = '$inServerID';";
    $result = ls_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );

    if( $numRows < 1 ) {
        return "unknown";
        }

    return ls_mysqli_result( $result, 0, "server" );
    }





function ls_getGeneration( $inLifeID ) {
    global $tableNamePrefix;
    
    $query = "SELECT server_id, generation, parent_id ".
        "FROM $tableNamePrefix"."lives ".
        "WHERE id = '$inLifeID';";

    $result = ls_queryDatabase( $query );

    $generation = ls_mysqli_result( $result, 0, "generation" );
    
    if( $generation == -1 ) {

        // compute it, if we can

        $server_id = ls_mysqli_result( $result, 0, "server_id" );
        $parent_id = ls_mysqli_result( $result, 0, "parent_id" );

        $tempGen = 0;
        
        while( $parent_id != -1 ) {
            $tempGen++;

            $parent_life_id = ls_getLifeID( $server_id, $parent_id );
            
            if( $parent_life_id == -1 ) {
                // parent hasn't died yet
                break;
                }
            
            $query = "SELECT generation, parent_id ".
                "FROM $tableNamePrefix"."lives ".
                "WHERE id = '$parent_life_id';";

            $result = ls_queryDatabase( $query );

            $parent_id = ls_mysqli_result( $result, 0, "parent_id" );
            $parentGen = ls_mysqli_result( $result, 0, "generation" );

            if( $parentGen != -1 ) {
                $generation = $parentGen + $tempGen;
                }
            }

        if( $generation != -1 ) {
            // found it
            // save it

            $query = "UPDATE $tableNamePrefix"."lives SET ".
                "generation = '$generation' WHERE id = '$inLifeID';";
            ls_queryDatabase( $query );
            }
        }

    return $generation;
    }




function ls_getEveID( $inLifeID ) {
    global $tableNamePrefix;
    
    $query = "SELECT server_id, eve_life_id, parent_id ".
        "FROM $tableNamePrefix"."lives ".
        "WHERE id = '$inLifeID';";

    $result = ls_queryDatabase( $query );

    $eve_life_id = ls_mysqli_result( $result, 0, "eve_life_id" );
    
    if( $eve_life_id == -1 ) {

        // compute it, if we can

        $server_id = ls_mysqli_result( $result, 0, "server_id" );
        $parent_id = ls_mysqli_result( $result, 0, "parent_id" );

        
        while( $parent_id != -1 ) {

            $parent_life_id = ls_getLifeID( $server_id, $parent_id );
            
            if( $parent_life_id == -1 ) {
                // parent hasn't died yet
                break;
                }
            
            $query = "SELECT id, eve_life_id, parent_id ".
                "FROM $tableNamePrefix"."lives ".
                "WHERE id = '$parent_life_id';";

            $result = ls_queryDatabase( $query );

            $parent_id = ls_mysqli_result( $result, 0, "parent_id" );
            $eve_life_id = ls_mysqli_result( $result, 0, "eve_life_id" );

            if( $eve_life_id == 0 || $parent_id == -1 ) {
                // reached Eve
                $eve_life_id = ls_mysqli_result( $result, 0, "id" );
                break;
                }
            if( $eve_life_id > 0 ) {
                // found an ancestor who is already marked with Eve
                break;
                }
            }

        if( $eve_life_id != -1 ) {
            // found it
            // save it

            $query = "UPDATE $tableNamePrefix"."lives SET ".
                "eve_life_id = '$eve_life_id' WHERE id = '$inLifeID';";
            ls_queryDatabase( $query );
            }
        }

    return $eve_life_id;
    }




// cannot be called if record doesn't exist yet
function ls_getUserID( $inEmail ) {
    global $tableNamePrefix;
    
    $query = "SELECT id FROM $tableNamePrefix"."users ".
        "WHERE email = '$inEmail';";
    $result = ls_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );

    if( $numRows < 1 ) {
        return -1;
        }

    return ls_mysqli_result( $result, 0, "id" );
    }


// gets from a player_id as logged by the server 
function ls_getLifeID( $inServerID, $inPlayerID ) {
    global $tableNamePrefix;
    
    $query = "SELECT id FROM $tableNamePrefix"."lives ".
        "WHERE server_id = '$inServerID' AND player_id = '$inPlayerID';";
    $result = ls_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );

    if( $numRows < 1 ) {
        return -1;
        }

    return ls_mysqli_result( $result, 0, "id" );
    }





function ls_getMale( $inLifeID ) {
    global $tableNamePrefix;
    
    $query = "SELECT male FROM $tableNamePrefix"."lives ".
        "WHERE id = '$inLifeID';";
    $result = ls_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );

    if( $numRows < 1 ) {
        return 0;
        }

    return ls_mysqli_result( $result, 0, "male" );
    }






function ls_getBirthSecAgo( $inLifeID ) {
    global $tableNamePrefix;

    $query = "SELECT death_time, age FROM $tableNamePrefix"."lives ".
        "WHERE id = '$inLifeID';";
    
    $result = ls_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );

    if( $numRows < 1 ) {
        return 0;
        }

    $deathAgoSec =
        strtotime( "now" ) -
        strtotime( ls_mysqli_result( $result, 0, "death_time" ) );

    $birthAgoSec = $deathAgoSec + ls_mysqli_result( $result, 0, "age" ) * 60;
    
    return $birthAgoSec;
    }



function ls_getDeathSecAgo( $inLifeID ) {
    global $tableNamePrefix;

    $query = "SELECT death_time FROM $tableNamePrefix"."lives ".
        "WHERE id = '$inLifeID';";
    
    $result = ls_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );

    if( $numRows < 1 ) {
        return 0;
        }

    $deathAgoSec =
        strtotime( "now" ) -
        strtotime( ls_mysqli_result( $result, 0, "death_time" ) );
  
    return $deathAgoSec;
    }




function ls_getDisplayID( $inLifeID ) {
    global $tableNamePrefix;
    
    $query = "SELECT display_id FROM $tableNamePrefix"."lives ".
        "WHERE id = '$inLifeID';";
    
    $result = ls_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );

    if( $numRows < 1 ) {
        return 0;
        }
    return ls_mysqli_result( $result, 0, "display_id" );
    }



// gets life_id of one child
function ls_getAllChildren( $inServerID, $inParentID ) {
    global $tableNamePrefix;

    // order by birth time
    $query = "SELECT id FROM $tableNamePrefix"."lives ".
        "WHERE server_id = '$inServerID' AND parent_id = '$inParentID' ".
        "ORDER BY DATE_SUB( ".
        "  death_time, INTERVAL floor( age * 60 ) SECOND ) ASC;";
    
    $result = ls_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );

    if( $numRows < 1 ) {
        return array();
        }

    $children = array();
    
    for( $i=0; $i<$numRows; $i++ ) {
        $children[$i] = ls_mysqli_result( $result, $i, "id" );
        }
    return $children;
    }




// returns -1 on failure
// found here:
// https://stackoverflow.com/questions/6265596/
//         how-to-convert-a-roman-numeral-to-integer-in-php
function ls_romanToInt( $inRomanString ) {    

    $romans = array(
        // currently, server never uses M or D
        /*'M' => 1000,
          'CM' => 900,
          'D' => 500,
          'CD' => 400,
        */
        'C' => 100,
        'XC' => 90,
        'L' => 50,
        'XL' => 40,
        'X' => 10,
        'IX' => 9,
        'V' => 5,
        'IV' => 4,
        'I' => 1 );

    $result = 0;

    foreach( $romans as $key => $value ) {
        while( strpos( $inRomanString, $key ) === 0 ) {
            $result += $value;
            $inRomanString = substr( $inRomanString, strlen( $key ) );
            }
        }

    if( strlen( $inRomanString ) == 0 ) {
        return $result;
        }
    else {
        // unmatched characters left
        return -1;
        }
    }



function ls_formatName( $inName ) {
    $inName = strtoupper( $inName );

    $nameParts = preg_split( "/\s+/", $inName );

    $numParts = count( $nameParts );

    if( $numParts > 0 ) {
        // first part always a name part,
        $nameParts[0] = ucfirst( strtolower( $nameParts[0] ) );
        }
    
    if( $numParts == 3 ) {
        // second part last name
        $nameParts[1] = ucfirst( strtolower( $nameParts[1] ) );
        // leave suffix uppercase
        }
    else if( count( $nameParts ) == 2 ) {
        // tricky case
        // is second part suffix or last name?

        $secondRoman = false;
        
        if( !preg_match('/[^IVCXL]/', $nameParts[1] ) ) {
            // string contains only roman numeral digits
            // but VIX and other last names possible
            if( ls_romanToInt( $nameParts[1] ) > 0 ) {
                // leave second part uppercase
                $secondRoman = true;
                }
            }

        if( ! $secondRoman ) {
            // second is last name
            $nameParts[1] = ucfirst( strtolower( $nameParts[1] ) );
            }
        }
    
    return implode( " ", $nameParts );
    }




function ls_logLife() {
    global $tableNamePrefix, $sharedGameServerSecret;

    // no locking is done here, because action is asynchronous anyway
    // and there's no way to prevent a server from acting on a stale
    // sequence number if calls for the same email are interleaved

    // however, we only want to support a given email address playing on
    // one server at a time, so it's okay if some parallel lives are
    // not logged correctly
    

    $server = ls_requestFilter( "server", "/[A-Z0-9.\-]+/i", "" );
    $email = ls_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i", "" );
    $age = ls_requestFilter( "age", "/[0-9.]+/i", "0" );

    $player_id = ls_requestFilter( "player_id", "/[0-9]+/i", "0" );
    $parent_id = ls_requestFilter( "parent_id", "/[0-9\-]+/i", "-1" );
    $killer_id = ls_requestFilter( "killer_id", "/[0-9\-]+/i", "-1" );
    $display_id = ls_requestFilter( "display_id", "/[0-9]+/i", "0" );

    $name = ls_requestFilter( "name", "/[A-Z ]+/i", "" );
    $last_words = ls_requestFilter(
        "last_words", "/[ABCDEFGHIJKLMNOPQRSTUVWXYZ.\-,'?! ]+/i", "" );

    // force sentence capitalization and spaces after end punctuation
    // found here:
    // https://stackoverflow.com/questions/5383471/
    //         how-to-capitalize-first-letter-of-first-word-in-a-sentence
    $last_words =
        preg_replace_callback( '/([.!?])\s*(\w)/',
                               function ($matches) {
                                   return strtoupper( $matches[1] .
                                                      '  ' .
                                                      $matches[2] );
                                   },
                               ucfirst( strtolower( $last_words ) ) );
    

    
    
    
    
    $name = ls_formatName( $name );
    
    $male = ls_requestFilter( "male", "/[01]/", "0" );
    
    $sequence_number = ls_requestFilter( "sequence_number", "/[0-9]+/i", "0" );

    $hash_value = ls_requestFilter( "hash_value", "/[A-F0-9]+/i", "" );

    $hash_value = strtoupper( $hash_value );


    if( $email == "" ||
        $server == "" ) {

        ls_log( "logLife denied for bad email or server name" );
        
        echo "DENIED";
        return;
        }
    
    $trueSeq = ls_getSequenceNumberForEmail( $email );

    if( $trueSeq > $sequence_number ) {
        ls_log( "logLife denied for stale sequence number" );

        echo "DENIED";
        return;
        }

    $computedHashValue =
        strtoupper( ls_hmac_sha1( $sharedGameServerSecret, $sequence_number ) );

    if( $computedHashValue != $hash_value ) {
        ls_log( "logLife denied for bad hash value" );

        echo "DENIED";
        return;
        }

    ls_log( "Got valid logLife call:  " . $_SERVER[ 'QUERY_STRING' ] );
    
    
    if( $trueSeq == 0 ) {
        // no record exists, add one
        $query = "INSERT INTO $tableNamePrefix". "users SET " .
            "email = '$email', ".
            "email_sha1 = sha1( '$email' ), ".
            "sequence_number = 1, ".
            "life_count = 1 ".
            "ON DUPLICATE KEY UPDATE sequence_number = sequence_number + 1, ".
            "life_count = life_count + 1;";
        }
    else {
        // update the existing one
        $query = "UPDATE $tableNamePrefix"."users SET " .
            // our values might be stale, increment values in table
            "sequence_number = sequence_number + 1, ".
            "life_count = life_count + 1 " .
            "WHERE email = '$email'; ";
        
        }

    ls_queryDatabase( $query );

    // now log life details
    
    $server_id = ls_getServerID( $server );
    $user_id = ls_getUserID( $email );

    // unknown until we are asked to compute it the first time
    $generation = -1;
    $eve_life_id = -1;
    
    if( $parent_id == -1 ) {
        // Eve
        // we know it
        $generation = 1;
        $eve_life_id = 0;
        }
    
    
    $query = "INSERT INTO $tableNamePrefix". "lives SET " .
        "death_time = CURRENT_TIMESTAMP, ".
        "server_id = '$server_id', ".
        "user_id = '$user_id', ".
        "player_id = '$player_id', ".
        "parent_id = '$parent_id', ".
        "killer_id = '$killer_id', ".
        // generate this later, as-needed
        "death_cause = '', ".
        "display_id = '$display_id', ".
        "age = '$age', ".
        "name = '$name', ".
        "male = '$male', ".
        // double-quotes, because ' is an allowed character
        "last_words = \"$last_words\", ".
        "generation = '$generation', " .
        "eve_life_id = '$eve_life_id', ".
        "deepest_descendant_generation = -1, ".
        "deepest_descendant_life_id = -1;";

    ls_queryDatabase( $query );

    $life_id = ls_getLifeID( $server_id, $player_id );
    $deepestInfo = ls_computeDeepestGeneration( $life_id );


    $deepest_descendant_generation = $deepestInfo[0];
    $deepest_descendant_life_id = $deepestInfo[1];

    if( $deepest_descendant_generation <= 0 ) {

        $deepest_descendant_generation = ls_getGeneration( $life_id );
        $deepest_descendant_life_id = $life_id;
        }
    
    
    if( $deepest_descendant_generation > 0 ) {
        // have generation info for this person

        // walk up and set deepest generation for all ancestors

        $parentLifeID = ls_getParentLifeID( $life_id );

        if( $parentLifeID != -1 ) {
            ls_setDeepestGenerationUp( $parentLifeID,
                                       $deepest_descendant_generation,
                                       $deepest_descendant_life_id );
            }
        }
    
    
    echo "OK";
    }




function ls_setDeepestGenerationUp( $inID,
                                    $in_deepest_descendant_generation,
                                    $in_deepest_descendant_life_id ) {
    global $tableNamePrefix;
    
    $query = "SELECT parent_id, deepest_descendant_generation, ".
        "deepest_descendant_life_id ".
        "FROM $tableNamePrefix"."lives WHERE id=$inID;";
    
    $result = ls_queryDatabase( $query );
    
    $numRows = mysqli_num_rows( $result );

    if( $numRows == 0 ) {
        return;
        }

    $deepest_descendant_generation =
        ls_mysqli_result( $result, 0, "deepest_descendant_generation" );

    $deepest_descendant_life_id =
        ls_mysqli_result( $result, 0, "deepest_descendant_life_id" );

    if( $in_deepest_descendant_generation > $deepest_descendant_generation ) {
        // even deeper than last known

        // set it

        $query = "UPDATE $tableNamePrefix"."lives ".
            "SET ".
            "deepest_descendant_generation = ".
            "  $in_deepest_descendant_generation, ".
            "deepest_descendant_life_id = ".
            "  $in_deepest_descendant_life_id ".
            "WHERE id = $inID;";
        
        ls_queryDatabase( $query );

        // propagate it up
        $parentLifeID = ls_getParentLifeID( $inID );

        if( $parentLifeID != -1 ) {
            ls_setDeepestGenerationUp( $parentLifeID,
                                       $in_deepest_descendant_generation,
                                       $in_deepest_descendant_life_id );
            }
        }
    
    }




function ls_getFaceURLForAge( $inAge, $inDisplayID ) {
    $faceAges = array( 0, 4, 14, 30, 40, 55 );

    $faceAge = 0;
    if( $inAge > 2 && $inAge < 10 ) {
        $faceAge = 4;
        }
    else if( $inAge >= 10 && $inAge < 20 ) {
        $faceAge = 14;
        }
    else if( $inAge >= 20 && $inAge < 40 ) {
        $faceAge = 30;
        }
    else if( $inAge >= 40 && $inAge < 55 ) {
        $faceAge = 40;
        }
    else if( $inAge >= 55 ) {
        $faceAge = 55;
        }
    global $facesWebPath;
    
    return "$facesWebPath/face_".
        $inDisplayID."_".$faceAge.".png";
    }




function ls_frontPage() {

    $emailFilter =
        ls_requestFilter( "filter", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i", "" );

    $nameFilter = ls_requestFilter( "filter", "/[A-Z ]+/i", "" );

    $email_sha1 = ls_requestFilter( "email_sha1", "/[a-f0-9]+/i", "" );


    $filterClause = " WHERE 1 ";
    $filter = "";


    if( $email_sha1 != "" ) {
        $email_sha1 = strtolower( $email_sha1 );
        $filterClause = " WHERE users.email_sha1 = '$email_sha1' ";
        $filter = "[email hash]";
        }
    else if( $emailFilter != "" ) {
        $filterClause = " WHERE users.email = '$emailFilter' ";
        $filter = $emailFilter;
        }
    else if( $nameFilter != "" ) {
        // name filter is used as prefix filter for speed
        // there's no way to make a LIKE condition fast if there's a wild
        // card as the first character of the pattern (the entire table
        // needs to be scanned, and the index isn't used).
        //
        // A full-text index is another option, but probably overkill in
        // this case.
        $filterClause = " WHERE lives.name LIKE '$nameFilter%' ";
        $filter = $nameFilter;
        }

    

    global $header, $footer;

    eval( $header );

    echo "<center>";

    $filterToShow = $filter;
    

    if( ls_requestFilter( "hide_filter", "/[01]+/i", "0" ) ) {
        $filterToShow = "-hidden-";
        }
    
    // form for searching
?>
            <FORM ACTION="server.php" METHOD="post">
    <INPUT TYPE="hidden" NAME="action" VALUE="front_page">
             Email or Character Name:
    <INPUT TYPE="text" MAXLENGTH=40 SIZE=20 NAME="filter"
             VALUE="<?php echo $filterToShow;?>">
    <INPUT TYPE="Submit" VALUE="Filter">
    </FORM>
  
<?php

    
    echo "<table border=0 cellpadding=20>";

    global $usersPerPage;

    $numPerList = floor( $usersPerPage / 4 );

    echo "<tr><td colspan=6>".
        "<font size=5>Recent Elder Deaths:</font></td></tr>\n";

    ls_printFrontPageRows( "$filterClause AND age >= 50", "death_time DESC",
                           $numPerList );


    echo "<tr><td colspan=6><font size=5>Today's Long Lines:".
        "</font></td></tr>\n";
    
    ls_printFrontPageRows(
        "$filterClause AND death_time >= DATE_SUB( NOW(), INTERVAL 1 DAY )",
        "generation DESC, death_time DESC",
        $numPerList );
    
    
    echo "<tr><td colspan=6>".
        "<font size=5>Recent Adult Deaths:</font></td></tr>\n";

    ls_printFrontPageRows( "$filterClause AND age >= 20 AND age < 50",
                           "death_time DESC",
                           $numPerList );


    echo "<tr><td colspan=6>".
        "<font size=5>Recent Youth Deaths:</font></td></tr>\n";
    
    ls_printFrontPageRows( "$filterClause AND age < 20", "death_time DESC",
                           $numPerList );


    
    echo "<tr><td colspan=6><font size=5>This Week's Long Lines:".
        "</font></td></tr>\n";
    
    ls_printFrontPageRows(
        "$filterClause AND death_time >= DATE_SUB( NOW(), INTERVAL 1 WEEK )",
        "generation DESC, death_time DESC",
        $numPerList );

    

    echo "<tr><td colspan=6><font size=5>All-Time Long Lines:".
        "</font></td></tr>\n";
    
    ls_printFrontPageRows( $filterClause, "generation DESC, death_time DESC",
                           $numPerList );


    
    
    echo "</table></center>";
    
    
    eval( $footer );
    }



function ls_getGrayPercent( $inDeathAgoSec ) {
    // full gray in 7 hours, starting after 2 hours
    $grayPercent = floor( 20 * ( ( $inDeathAgoSec - 2 * 3600 ) / 3600 ) );
    
    if( $grayPercent > 100 ) {
        $grayPercent = 100;
        }
    else if( $grayPercent < 0 ) {
        $grayPercent = 0;
        }
    return $grayPercent;
    }



function ls_printFrontPageRows( $inFilterClause, $inOrderBy, $inNumRows ) {
    global $tableNamePrefix;


    $query = "SELECT lives.id, display_id, name, ".
        "age, generation, death_time ".
        "FROM $tableNamePrefix"."lives as lives ".
        "INNER JOIN $tableNamePrefix"."users as users ".
        "ON lives.user_id = users.id  $inFilterClause ".
        "ORDER BY $inOrderBy ".
        "LIMIT $inNumRows;";

    
    $result = ls_queryDatabase( $query );
    
    $numRows = mysqli_num_rows( $result );
    
    
    for( $i=0; $i<$numRows; $i++ ) {

        $id = ls_mysqli_result( $result, $i, "id" );
        $display_id = ls_mysqli_result( $result, $i, "display_id" );
        $name = ls_mysqli_result( $result, $i, "name" );
        $age = ls_mysqli_result( $result, $i, "age" );
        $generation = ls_mysqli_result( $result, $i, "generation" );
        $death_time = ls_mysqli_result( $result, $i, "death_time" );


        $deathAgoSec = strtotime( "now" ) -
            strtotime( $death_time );
        
        $deathAgo = ls_secondsToAgeSummary( $deathAgoSec );
        
        if( $generation == -1 ) {
            $generation = ls_getGeneration( $id );
            }
        if( $generation == -1 ) {
            if( $deathAgoSec >= 3600 ) {
                $generation = "Ancestor unknown";
                }
            else {
                $generation = "Mother still living";
                }
            }
        else {
            $generation = "Generation: $generation";
            }
        
        $age = floor( $age );

        $yearWord = "years";
        if( $age == 1 ) {
            $yearWord = "year";
            }
        
        
        
        echo "<tr>";

        $charLinkA =
            "<a href='server.php?action=character_page&id=$id'>";
        
        $faceURL = ls_getFaceURLForAge( $age, $display_id );

        $grayPercent = ls_getGrayPercent( $deathAgoSec );
        
        echo "<td></td>";
        
        echo "<td>".
            "$charLinkA<img src='$faceURL' ".
            "width=100 height=98 border=0 ".
            "style='-webkit-filter:sepia($grayPercent%);'></a></td>";

        echo "<td>$name</td>";
        echo "<td>$age $yearWord old</td>";
        echo "<td>$generation</td>";
        echo "<td>Died $deathAgo ago</td>";

        echo "</tr>";
        }

    }





// cache in each run
$lineageCache = array();

// gets array from $inFromID up to Eve, or limited by $inLimit steps
function ls_getLineage( $inFromID, $inLimit ) {
    global $lineageCache;

    if( in_array( $inFromID, $lineageCache ) ) {
        return $lineageCache[ $inFromID ];
        }
    

    $line = array();

    $parentID = $inFromID;

    $steps = 0;
    
    while( $parentID != -1 && $steps < $inLimit ) {
        $line[] = $parentID;
        $parentID = ls_getParentLifeID( $parentID );
        $steps++;
        }

    if( $inLimit > 10 ) {
        // store long ones to re-use later
        $lineageCache[ $inFromID ] = $line;
        }
    
    
    return $line;
    }




function ls_getCousinNumberWord( $inCousinNumber ) {
    switch( $inCousinNumber ) {
        case 1:
            return "First";
        case 2:
            return "Second";
        case 3:
            return "Third";
        case 4:
            return "Fourth";
        case 5:
            return "Fifth";
        case 6:
            return "Sixth";
        case 7:
            return "Seventh";
        case 8:
            return "Eigth";
        case 9:
            return "Ninth";
        case 10:
            return "Tenth";
        }
    
    // else use short form
    
    $onesDigit = $inCousinNumber % 10;

    $numSuffix = "th";
    if( $onesDigit == 1 ) {
        $numSuffix = "st";
        }
    else if( $onesDigit == 2 ) {
            $numSuffix = "nd";
        }
    else if( $onesDigit == 3 ) {
        $numSuffix = "rd";
        }
    
    return $inCousinNumber . $numSuffix;
    }


function ls_getCousinRemovedWord( $inRemovedSteps ) {
    switch( $inRemovedSteps ) {
        case 1:
            return "Once";
        case 2:
            return "Twice";
        case 3:
            return "Thrice";
        }
    return $inRemovedSteps . "x";
    }




// from is the person that we're taking the point of view of
// to is the person that we're trying to find the relationship name of
function ls_getRelName( $inFromID, $inToID, $inLimit ) {
    if( $inFromID == $inToID ) {
        return "";
        }

    $male = ls_getMale( $inToID );


    
    $fromLine = ls_getLineage( $inFromID, $inLimit );
    
    $parIndex = array_search( $inToID, $fromLine );

    if( $parIndex != FALSE ) {
        $rootWord = "Mother";
        if( $male ) {
            $rootWord = "Father";
            }
        if( $parIndex > 1 ) {
            $rootWord = "Grand" . strtolower( $rootWord );
            }
        $numGreats = $parIndex - 2;
        for( $i=0; $i<$numGreats; $i++ ) {
            $rootWord = "Great " . $rootWord;
            }
        return $rootWord;
        }


    
    $toLine = ls_getLineage( $inToID, $inLimit );
    
    $parIndex = array_search( $inFromID, $toLine );

    if( $parIndex != FALSE ) {
        $rootWord = "Daughter";
        if( $male ) {
            $rootWord = "Son";
            }
        if( $parIndex > 1 ) {
            $rootWord = "Grand" . strtolower( $rootWord );
            }
        $numGreats = $parIndex - 2;
        for( $i=0; $i<$numGreats; $i++ ) {
            $rootWord = "Great " . $rootWord;
            }
        return $rootWord;
        }

    // not direct descendents

    // walk up and find shared ancestor

    $sharedFromIndex = -1;

    $sharedToIndex = -1;

    $fromLineLen = count( $fromLine );
    $toLineLen = count( $toLine );

    for( $f=1; $f<$fromLineLen; $f++ ) {
        $fromAncestor = $fromLine[$f];
        
        for( $t=1; $t<$toLineLen; $t++ ) {

            if( $fromAncestor == $toLine[$t] ) {

                $sharedFromIndex = $f;
                $sharedToIndex = $t;
                break;
                }
            }

        if( $sharedFromIndex != -1 ) {
            break;
            }
        }

    if( $sharedFromIndex == -1 && $sharedToIndex == -1 &&
        ls_getParentID( $inFromID ) == ls_getParentID( $inToID )&&
        ls_getServerIDForLife( $inFromID ) ==
        ls_getServerIDForLife( $inToID ) ) {

        // same parent, but parent not dead yet
        $sharedFromIndex = 1;
        $sharedToIndex = 1;
        }
    
    
    if( $sharedFromIndex != -1 ) {
        // some relationship

        if( $sharedFromIndex == 1 && $sharedToIndex == 1 ) {
            // sibs
            $rootWord = "Sister";
            
            if( $male ) {
                $rootWord = "Brother";
                }
            $fromBirthTime = ls_getBirthSecAgo( $inFromID );
            $toBirthTime = ls_getBirthSecAgo( $inToID );

            if( $fromBirthTime < $toBirthTime - 10 ) {
                $rootWord = "Big $rootWord";
                }
            else if( $fromBirthTime > $toBirthTime + 10 ) {
                $rootWord = "Little $rootWord";
                }
            else {
                // close in age
                $rootWord = "Twin $rootWord";

                if( ls_getDisplayID( $inFromID ) ==
                    ls_getDisplayID( $inToID ) ) {
                    $rootWord = "Identical $rootWord";
                    }
                }
            return $rootWord;
            }

        if( $sharedToIndex == 1 ) {
            $rootWord = "Aunt";
            if( $male ) {
                $rootWord = "Uncle";
                }
            
            $numGreats = $sharedFromIndex - 2;
            for( $i=0; $i<$numGreats; $i++ ) {
                $rootWord = "Great " . $rootWord;
                }
            return $rootWord;
            }    

        if( $sharedFromIndex == 1 ) {
            $rootWord = "Niece";
            if( $male ) {
                $rootWord = "Nephew";
                }
            
            $numGreats = $sharedToIndex - 2;
            for( $i=0; $i<$numGreats; $i++ ) {
                $rootWord = "Great " . $rootWord;
                }
            return $rootWord;
            }    

        // some kind of cousin

        // shallowest determines cousin number
        // diff determines removed number
        $cousinNumber = $sharedToIndex;
        if( $sharedFromIndex < $cousinNumber ) {
            $cousinNumber = $sharedFromIndex;
            }
        $cousinNumber -= 1;

        $onesDigit = $cousinNumber % 10;

        $numSuffix = "th";
        if( $onesDigit == 1 ) {
            $numSuffix = "st";
            }
        else if( $onesDigit == 2 ) {
            $numSuffix = "nd";
            }
        else if( $onesDigit == 3 ) {
            $numSuffix = "rd";
            }

        $cousinName = ls_getCousinNumberWord( $cousinNumber ) . " Cousin";

        $numRemoved = abs( $sharedFromIndex - $sharedToIndex );

        if( $numRemoved > 0 ) {
            $cousinName = $cousinName . "<br>(" .
                ls_getCousinRemovedWord( $numRemoved ) . " Removed)";
            }
        
        return $cousinName;
        }
    
    

    
    return "No Relation";
    }



// returns array of previous generation
function ls_getPrevGen( $inFromID ) {
    $parentID = ls_getParentLifeID( $inFromID );

    if( $parentID != -1 ) {
        return ls_getSiblings( $parentID );
        }
    else {
        return array();
        }
    }



// returns array of siblings
function ls_getSiblings( $inFromID ) {
    $parentID = ls_getParentID( $inFromID );
    $serverID = ls_getServerIDForLife( $inFromID );

    if( $parentID == -1 ) {
        return array( $inFromID );
        }

    return ls_getAllChildren( $serverID, $parentID );
    }



// returns array of next generation
function ls_getNextGen( $inFromID ) {

    global $tableNamePrefix;
    
    $query = "SELECT server_id, player_id FROM $tableNamePrefix"."lives ".
        "WHERE id = '$inFromID';";
    $result = ls_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );

    if( $numRows < 1 ) {
        return array();
        }

    $server_id = ls_mysqli_result( $result, 0, "server_id" );
    $player_id = ls_mysqli_result( $result, 0, "player_id" );
    

    return ls_getAllChildren( $server_id, $player_id );
    }




function ls_displayPerson( $inID, $inRelID, $inFullWords ) {

    global $tableNamePrefix;

    $query = "SELECT id, display_id, name, ".
        "age, last_words, generation, death_time, death_cause ".
        "FROM $tableNamePrefix"."lives WHERE id=$inID;";
    
    $result = ls_queryDatabase( $query );
    
    $numRows = mysqli_num_rows( $result );

    if( $numRows == 1 ) {

        $id = ls_mysqli_result( $result, 0, "id" );
        $display_id = ls_mysqli_result( $result, 0, "display_id" );
        $name = ls_mysqli_result( $result, 0, "name" );
        $last_words = ls_mysqli_result( $result, 0, "last_words" );
        $age = ls_mysqli_result( $result, 0, "age" );
        $generation = ls_mysqli_result( $result, 0, "generation" );
        $death_time = ls_mysqli_result( $result, 0, "death_time" );
        $death_cause = ls_mysqli_result( $result, 0, "death_cause" );



        $deathAgoSec = strtotime( "now" ) -
            strtotime( $death_time );
        
        $deathAgo = ls_secondsToAgeSummary( $deathAgoSec );


        $age = floor( $age );
        
        
        $faceURL = ls_getFaceURLForAge( $age, $display_id );

        echo "<a href='server.php?action=character_page&".
            "id=$id&rel_id=$inRelID'>";

        $grayPercent = ls_getGrayPercent( $deathAgoSec );
        
        echo "<img src='$faceURL' ".
            "width=100 height=98 border=0 ".
            "style='-webkit-filter:sepia($grayPercent%);'>";

        echo "</a>";
        
        $yearWord = "years";
        if( $age == 1 ) {
            $yearWord = "year";
            }

        
        echo "<br>\n$name<br>\n";

        // most common case requires only a few steps
        // don't walk all the way up unless we have to
        $relName = ls_getRelName( $inRelID, $inID, 3 );

        if( $relName == "No Relation" ) {
            // allow more steps, but still don't walk all the way to
            // the top of deep trees
            $relName = ls_getRelName( $inRelID, $inID, 25 );

            if( $relName == "No Relation" ) {
                $relName = "Distant Relative";
                }
            }
        

        if( $relName != "" ) {
            echo "$relName<br>\n";
            }
        echo "$age $yearWord old<br>\n";
        echo "$deathAgo ago\n";

        if( ! $inFullWords ) {
            if( strlen( $last_words ) > 18 ) {
                $last_words = trim( substr( $last_words, 0, 16 ) );
                $lastChar = substr( $last_words, -1 );
                
                if( $lastChar != '.' &&
                    $lastChar != '!' &&
                    $lastChar != '?' ) {
                    
                    $last_words = $last_words . "...";
                    }
                }
            }

        $deathHTML = $death_cause;

        if( $deathHTML == "" ) {
            $deathHTML = ls_getDeathHTML( $inID, $inRelID );
            }

        if( $deathHTML != "" ) {
            echo "<br>\n";
            echo "$deathHTML\n";
            }

            
        
        if( $last_words != "" ) {
            echo "<br>\n";
            echo "Final words: \"$last_words\"\n";
            }
        }
    
    }



function ls_displayGenRow( $inGenArray, $inCenterID, $inRelID, $inFullWords ) {


    $full = $inGenArray;


    $count = count( $full );

    if( $count > 0 ) {
        $gen = ls_getGeneration( $full[0] );

        $genString;

        if( $gen == -1 ) {
            if( ls_getDeathSecAgo( $full[0] ) >= 3600 ) {
                $genString = "Generation ? (Ancestor unknown):";
                }
            else {
                $genString = "Generation ? (Mother still living):";
                }
            }
        else {
            $genString = "Generation $gen:";
            }
        
        echo "<center><font size=5>$genString</font></center>\n";
        }
    
    
    echo "<table border=0 cellpadding=20><tr>\n";

    
    
    for( $i=0; $i<$count; $i++ ) {
        $bgColorString = "";
        if( $full[$i] == $inCenterID ) {
            $bgColorString = "bgcolor=#444444";
            }
        else {
            $bgColorString = "bgcolor=#222222";
            }
        
        
        echo "<td valign=top align=center $bgColorString>\n";
        
        ls_displayPerson( $full[$i], $inRelID,
                          $inFullWords &&
                          ( $full[$i] == $inCenterID ) );
        echo "</td>\n";
        }
    echo "</tr></table>\n";
    }




function ls_getDeathHTML( $inID, $inRelID ) {
    global $tableNamePrefix;
    
    $query = "SELECT age, killer_id, server_id, death_cause ".
        "FROM $tableNamePrefix"."lives WHERE id=$inID;";
    
    $result = ls_queryDatabase( $query );
    
    $numRows = mysqli_num_rows( $result );

    if( $numRows == 0 ) {
        return "";
        }

    $death_cause = ls_mysqli_result( $result, 0, "death_cause" );

    if( $death_cause != "" ) {
        return $death_cause;
        }


    $killer_id = ls_mysqli_result( $result, 0, "killer_id" );
    $server_id = ls_mysqli_result( $result, 0, "server_id" );
    $age = ls_mysqli_result( $result, 0, "age" );
    
    
    if( $killer_id > 0 ) {
        $query = "SELECT id, name ".
            "FROM $tableNamePrefix"."lives WHERE player_id=$killer_id ".
            "AND server_id=$server_id;";
        
        $result = ls_queryDatabase( $query );
        
        $numRows = mysqli_num_rows( $result );
        
        if( $numRows == 0 ) {
            return "Murdered";
            }

        $id = ls_mysqli_result( $result, 0, "id" );
        $name = ls_mysqli_result( $result, 0, "name" );        

        if( $id == $inID ) {
            // killed self
            return "Sudden Infant Death";
            }
        else {
            return "Killed by <a href='server.php?action=character_page&".
                "id=$id&rel_id=$inRelID'>$name</a>";
            }
        }
    else if( $killer_id <= -1 ) {

        $deathString = "";
        
        if( $killer_id == -1 ) {
            if( $age >= 60 ) {
                $deathString = "Died of Old Age";
                }
            else {
                $deathString = "Starved";
                }
            }
        else {
            global $objectsPath;

            $objID = - $killer_id;

            $fileName = $objectsPath . "/" . $objID . ".txt";

            if( file_exists( $fileName ) ) {

                $fh = fopen( $fileName, 'r');
                $line = fgets( $fh );
                if( $line != FALSE ) {
                    // second line is description
                    $line = fgets( $fh );
                    if( $line != FALSE ) {
                        $commentPos = strpos( $line, "#" );
                        if( $commentPos != FALSE ) {
                            $line = substr( $line, 0, $commentPos );
                            }

                        $deathString = "Killed by $line";
                        }
                    }
                
                fclose( $fh );
                }
            }
        if( $deathString != "" ) {
            // save it
            
            $query = "UPDATE $tableNamePrefix"."lives ".
                "SET death_cause='$deathString' ".
                "WHERE id=$inID;";
            ls_queryDatabase( $query );
            }
        
        return $deathString;
        }
    }



// walks down tree
// returns array  of
// { deepest_descendant_generation, deepest_descendant_life_id }
function ls_computeDeepestGeneration( $inID ) {
    global $tableNamePrefix;
    
    $query = "SELECT deepest_descendant_generation, ".
        "deepest_descendant_life_id ".
        "FROM $tableNamePrefix"."lives WHERE id=$inID;";
    
    $result = ls_queryDatabase( $query );
    
    $numRows = mysqli_num_rows( $result );

    if( $numRows == 0 ) {
        return array( -1, -1 );
        }

    $deepest_descendant_generation =
        ls_mysqli_result( $result, 0, "deepest_descendant_generation" );

    $deepest_descendant_life_id =
        ls_mysqli_result( $result, 0, "deepest_descendant_life_id" );

    if( $deepest_descendant_generation == -1 ) {

        $deepest_descendant_generation = 0;
        $deepest_descendant_life_id = 0;
        
        $nextGen = ls_getNextGen( $inID );

        $numNext = count( $nextGen );

        
        if( $numNext > 0 ) {
            $deepest_descendant_generation = ls_getGeneration( $nextGen[0] );
            $deepest_descendant_life_id = $nextGen[0];
            }
        
        for( $i=0; $i<$numNext; $i++ ) {
            $next = $nextGen[$i];

            $nextDeep = ls_computeDeepestGeneration( $next );

            if( $nextDeep[0] > $deepest_descendant_generation ) {
                $deepest_descendant_generation = $nextDeep[0];
                $deepest_descendant_life_id = $nextDeep[1];
                }
            }

        $query = "UPDATE $tableNamePrefix"."lives ".
            "SET ".
            "deepest_descendant_generation = $deepest_descendant_generation, ".
            "deepest_descendant_life_id = $deepest_descendant_life_id ".
            "WHERE id = $inID;";
        
        ls_queryDatabase( $query );  
        }
    
    return array( $deepest_descendant_generation, $deepest_descendant_life_id );
    }




function ls_getParentLifeID( $inID ) {
    global $tableNamePrefix;
    
    $query = "SELECT server_id, parent_id ".
        "FROM $tableNamePrefix"."lives WHERE id=$inID;";
    
    $result = ls_queryDatabase( $query );
    
    $numRows = mysqli_num_rows( $result );

    if( $numRows == 0 ) {
        return -1;
        }

    $parent_id = ls_mysqli_result( $result, 0, "parent_id" );

    if( $parent_id == -1 ) {
        return -1;
        }
    
    return ls_getLifeID( ls_mysqli_result( $result, 0, "server_id" ),
                         $parent_id );
    }



function ls_getParentID( $inID ) {
    global $tableNamePrefix;
    
    $query = "SELECT parent_id ".
        "FROM $tableNamePrefix"."lives WHERE id=$inID;";
    
    $result = ls_queryDatabase( $query );
    
    $numRows = mysqli_num_rows( $result );

    if( $numRows == 0 ) {
        return -1;
        }

    return ls_mysqli_result( $result, 0, "parent_id" );
    }



function ls_getServerIDForLife( $inLifeID ) {
    global $tableNamePrefix;
    
    $query = "SELECT server_id ".
        "FROM $tableNamePrefix"."lives WHERE id=$inLifeID;";
    
    $result = ls_queryDatabase( $query );
    
    $numRows = mysqli_num_rows( $result );

    if( $numRows == 0 ) {
        return -1;
        }

    return ls_mysqli_result( $result, 0, "server_id" );
    }




function ls_getLifeExists( $inID ) {
    global $tableNamePrefix;
    
    $query = "SELECT id ".
        "FROM $tableNamePrefix"."lives WHERE id=$inID;";
    
    $result = ls_queryDatabase( $query );
    
    $numRows = mysqli_num_rows( $result );

    if( $numRows == 0 ) {
        return false;
        }
    return true;
    }




function ls_characterPage() {

    $id = ls_requestFilter( "id", "/[0-9]+/i", "0" );

    $rel_id = ls_requestFilter( "rel_id", "/[0-9]+/i", "0" );

    if( $rel_id == 0 ) {
        $rel_id = $id;
        }


    
    
    
    //echo "ID = $id and relID = $rel_id<br>";
    
    global $header, $footer;

    eval( $header );

    echo "<center>\n";

    if( ! ls_getLifeExists( $id )
        ||
        ( $rel_id != $id && ! ls_getLifeExists( $rel_id ) ) ) {

        echo "Not found";
        echo "</center>\n";
        eval( $footer );
        return;
        }
    
    $parent = ls_getParentLifeID( $id );
    $ancestor = ls_getEveID( $id );

    if( $ancestor > 0 && $ancestor != $parent ) {
        $ancientGen = ls_getSiblings( $ancestor );
        // ancestor in center
        ls_displayGenRow( $ancientGen, ls_getParentLifeID( $ancestor ),
                          $rel_id, false );

        echo "<font size=5>...</font>\n";
        }
    
    
    $prevGen = ls_getPrevGen( $id );
    // parent in center
    ls_displayGenRow( $prevGen, ls_getParentLifeID( $id ), $rel_id, false );

    //echo "This gen<br>";
    
    $sibs = ls_getSiblings( $id );
    // target in center and display full words for target
    ls_displayGenRow( $sibs, $id, $rel_id, true );

    
    $nextGen = ls_getNextGen( $id );
    // no one needs to be in center of next gen
    ls_displayGenRow( $nextGen, -1, $rel_id, false );


    if( count( $nextGen ) > 0 ) {
        $gen = ls_getGeneration( $nextGen[0] );
    
        $deepestGenInfo = ls_computeDeepestGeneration( $id );
        
        $deepest_descendant_generation = $deepestGenInfo[0];
        $deepest_descendant_life_id = $deepestGenInfo[1];
        
        if( $deepest_descendant_generation > 0 &&
            $deepest_descendant_generation > $gen ) {

            if( $deepest_descendant_generation > $gen + 1 ) {    
                echo "<font size=5>...</font>\n";
                }
            
            $deep_sibs = ls_getSiblings( $deepest_descendant_life_id );
            // target in center and display full words for target
            ls_displayGenRow( $deep_sibs, -1, $rel_id, false );
            }
        }
    

    
    echo "</center>\n";

    eval( $footer );    
    }





function ls_characterDump() {
    global $tableNamePrefix;
    
    $id = ls_requestFilter( "id", "/[0-9]+/i", "0" );


    if( $id > 0 ) {
        $query = "SELECT * FROM $tableNamePrefix"."lives ".
            "WHERE id = $id;";

        $result = ls_queryDatabase( $query );

        $numRows = mysqli_num_rows( $result );

        if( $numRows == 1 ) {

            $death_time = ls_mysqli_result( $result, 0, "death_time" );
            $server_id = ls_mysqli_result( $result, 0, "server_id" );

            $parent_id = ls_mysqli_result( $result, 0, "parent_id" );

            $parentLifeID = -1;

            if( $parent_id != -1 ) {
                $parentLifeID = ls_getLifeID( $server_id, $parent_id );
                }

            $killer_id = ls_mysqli_result( $result, 0, "killer_id" );

            $killerLifeID = -1;

            if( $killer_id > 0 ) {
                $killerLifeID = ls_getLifeID( $server_id, $killer_id );
                }
            
            $death_cause = ls_mysqli_result( $result, 0, "death_cause" );

            $display_id = ls_mysqli_result( $result, 0, "display_id" );

            $name = ls_mysqli_result( $result, 0, "name" );

            $age = ls_mysqli_result( $result, 0, "age" );
            $last_words = ls_mysqli_result( $result, 0, "last_words" );
            
            $male = ls_mysqli_result( $result, 0, "male" );

            $serverName = ls_getServerName( $server_id );

            echo "death_time = $death_time\n";
            echo "server_name = $serverName\n";
            echo "parent_id = $parentLifeID\n";
            echo "killer_id = $killerLifeID\n";
            echo "death_cause = $death_cause\n";
            echo "display_id = $display_id\n";
            echo "name = $name\n";
            echo "age = $age\n";
            echo "last_words = $last_words\n";
            echo "male = $male";
            }        
        }    
    }


function ls_reformatNames() {
    global $tableNamePrefix;

    $query = "SELECT id, name FROM $tableNamePrefix"."lives ".
            "WHERE death_time > '2018-07-06';";

    $result = ls_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );

    echo "Processing $numRows names to reformat...<br>\n";

    $touched = 0;
    
    for( $i=0; $i<$numRows; $i++ ) {
        
        $id = ls_mysqli_result( $result, $i, "id" );

        
        $name = ls_mysqli_result( $result, $i, "name" );

        $oldName = $name;
        

        $name = ls_formatName( $name );

        if( $oldName != $name ) {
            echo "Old name:  $oldName :::: ";
            echo "New name:  $name<br>\n";
            $query = "UPDATE $tableNamePrefix"."lives SET ".
                "name = '$name' WHERE id = '$id';";
            ls_queryDatabase( $query );
            $touched ++;
            }
        }

    $notTouched = $numRows - $touched;
    
    echo "$touched/$numRows needed to be updated.  Done<br>\n";
    }

















$ls_mysqlLink;


// general-purpose functions down here, many copied from seedBlogs

/**
 * Connects to the database according to the database variables.
 */  
function ls_connectToDatabase() {
    global $databaseServer,
        $databaseUsername, $databasePassword, $databaseName,
        $ls_mysqlLink;
    
    
    $ls_mysqlLink =
        mysqli_connect( $databaseServer, $databaseUsername, $databasePassword )
        or ls_operationError( "Could not connect to database server: " .
                              mysqli_error( $ls_mysqlLink ) );
    
    mysqli_select_db( $ls_mysqlLink, $databaseName )
        or ls_operationError( "Could not select $databaseName database: " .
                              mysqli_error( $ls_mysqlLink ) );
    }


 
/**
 * Closes the database connection.
 */
function ls_closeDatabase() {
    global $ls_mysqlLink;
    
    mysqli_close( $ls_mysqlLink );
    }


/**
 * Returns human-readable summary of a timespan.
 * Examples:  10.5 hours
 *            34 minutes
 *            45 seconds
 */
function ls_secondsToTimeSummary( $inSeconds ) {
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
function ls_secondsToAgeSummary( $inSeconds ) {
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
function ls_queryDatabase( $inQueryString ) {
    global $ls_mysqlLink;
    
    if( gettype( $ls_mysqlLink ) != "resource" ) {
        // not a valid mysql link?
        ls_connectToDatabase();
        }
    
    $result = mysqli_query( $ls_mysqlLink, $inQueryString );
    
    if( $result == FALSE ) {

        $errorNumber = mysqli_errno( $ls_mysqlLink );
        
        // server lost or gone?
        if( $errorNumber == 2006 ||
            $errorNumber == 2013 ||
            // access denied?
            $errorNumber == 1044 ||
            $errorNumber == 1045 ||
            // no db selected?
            $errorNumber == 1046 ) {

            // connect again?
            ls_closeDatabase();
            ls_connectToDatabase();

            $result = mysqli_query( $ls_mysqlLink, $inQueryString )
                or ls_operationError(
                    "Database query failed:<BR>$inQueryString<BR><BR>" .
                    mysqli_error( $ls_mysqlLink ) );
            }
        else {
            // some other error (we're still connected, so we can
            // add log messages to database
            ls_fatalError( "Database query failed:<BR>$inQueryString<BR><BR>" .
                           mysqli_error( $ls_mysqlLink ) );
            }
        }

    return $result;
    }



/**
 * Replacement for the old mysql_result function.
 */
function ls_mysqli_result( $result, $number, $field=0 ) {
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
function ls_doesTableExist( $inTableName ) {
    // check if our table exists
    $tableExists = 0;
    
    $query = "SHOW TABLES";
    $result = ls_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );


    for( $i=0; $i<$numRows && ! $tableExists; $i++ ) {

        $tableName = ls_mysqli_result( $result, $i, 0 );
        
        if( $tableName == $inTableName ) {
            $tableExists = 1;
            }
        }
    return $tableExists;
    }



function ls_log( $message ) {
    global $enableLog, $tableNamePrefix, $ls_mysqlLink;

    if( $enableLog ) {
        $slashedMessage = mysqli_real_escape_string( $ls_mysqlLink, $message );
    
        $query = "INSERT INTO $tableNamePrefix"."log VALUES ( " .
            "'$slashedMessage', CURRENT_TIMESTAMP );";
        $result = ls_queryDatabase( $query );
        }
    }



/**
 * Displays the error page and dies.
 *
 * @param $message the error message to display on the error page.
 */
function ls_fatalError( $message ) {
    //global $errorMessage;

    // set the variable that is displayed inside error.php
    //$errorMessage = $message;
    
    //include_once( "error.php" );

    // for now, just print error message
    $logMessage = "Fatal error:  $message";
    
    echo( $logMessage );

    ls_log( $logMessage );
    
    die();
    }



/**
 * Displays the operation error message and dies.
 *
 * @param $message the error message to display.
 */
function ls_operationError( $message ) {
    
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
function ls_addslashes_deep( $inValue ) {
    return
        ( is_array( $inValue )
          ? array_map( 'ls_addslashes_deep', $inValue )
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
function ls_stripslashes_deep( $inValue ) {
    return
        ( is_array( $inValue )
          ? array_map( 'ls_stripslashes_deep', $inValue )
          : stripslashes( $inValue ) );
    }



/**
 * Filters a $_REQUEST variable using a regex match.
 *
 * Returns "" (or specified default value) if there is no match.
 */
function ls_requestFilter( $inRequestVariable, $inRegex, $inDefault = "" ) {
    if( ! isset( $_REQUEST[ $inRequestVariable ] ) ) {
        return $inDefault;
        }

    return ls_filter( $_REQUEST[ $inRequestVariable ], $inRegex, $inDefault );
    }


/**
 * Filters a value  using a regex match.
 *
 * Returns "" (or specified default value) if there is no match.
 */
function ls_filter( $inValue, $inRegex, $inDefault = "" ) {
    
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
function ls_checkPassword( $inFunctionName ) {
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
        $password = ls_hmac_sha1( $passwordHashingPepper,
                                  $_REQUEST[ "passwordHMAC" ] );
        
        
        // generate a new hash cookie from this password
        $newSalt = time();
        $newHash = md5( $newSalt . $password );
        
        $password_hash = $newSalt . "_" . $newHash;
        }
    else if( isset( $_COOKIE[ $cookieName ] ) ) {
        ls_checkReferrer();
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

            ls_log( "Failed $inFunctionName access with password:  ".
                    "$password" );
            }
        else {
            echo "Session expired.";
                
            ls_log( "Failed $inFunctionName access with bad cookie:  ".
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
            
            
            $nonce = ls_hmac_sha1( $ticketGenerationSecret, uniqid() );
            
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
 



function ls_clearPasswordCookie() {
    global $tableNamePrefix;

    $cookieName = $tableNamePrefix . "cookie_password_hash";

    // expire 24 hours ago (to avoid timezone issues)
    $expireTime = time() - 60 * 60 * 24;

    setcookie( $cookieName, "", $expireTime, "/" );
    }
 
 







function ls_hmac_sha1( $inKey, $inData ) {
    return hash_hmac( "sha1", 
                      $inData, $inKey );
    } 

 
function ls_hmac_sha1_raw( $inKey, $inData ) {
    return hash_hmac( "sha1", 
                      $inData, $inKey, true );
    } 


 
 
 
 
// decodes a ASCII hex string into an array of 0s and 1s 
function ls_hexDecodeToBitString( $inHexString ) {
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
