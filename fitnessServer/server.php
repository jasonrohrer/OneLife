<?php



global $fs_version;
$fs_version = "1";


global $numLivesInAverage;
$numLivesInAverage = 10;



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
<HEAD><TITLE>Fitness Server Web-based setup</TITLE></HEAD>
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
    $_GET     = array_map( 'fs_stripslashes_deep', $_GET );
    $_POST    = array_map( 'fs_stripslashes_deep', $_POST );
    $_REQUEST = array_map( 'fs_stripslashes_deep', $_REQUEST );
    $_COOKIE  = array_map( 'fs_stripslashes_deep', $_COOKIE );
    }
    


// Check that the referrer header is this page, or kill the connection.
// Used to block XSRF attacks on state-changing functions.
// (To prevent it from being dangerous to surf other sites while you are
// logged in as admin.)
// Thanks Chris Cowan.
function fs_checkReferrer() {
    global $fullServerURL;
    
    if( !isset($_SERVER['HTTP_REFERER']) ||
        strpos($_SERVER['HTTP_REFERER'], $fullServerURL) !== 0 ) {
        
        die( "Bad referrer header" );
        }
    }




// all calls need to connect to DB, so do it once here
fs_connectToDatabase();

// close connection down below (before function declarations)


// testing:
//sleep( 5 );


// general processing whenver server.php is accessed directly




// grab POST/GET variables
$action = fs_requestFilter( "action", "/[A-Z_]+/i" );

$debug = fs_requestFilter( "debug", "/[01]/" );

$remoteIP = "";
if( isset( $_SERVER[ "REMOTE_ADDR" ] ) ) {
    $remoteIP = $_SERVER[ "REMOTE_ADDR" ];
    }




if( $action == "version" ) {
    global $fs_version;
    echo "$fs_version";
    }
else if( $action == "get_client_sequence_number" ) {
    fs_getClientSequenceNumber();
    }
else if( $action == "get_server_sequence_number" ) {
    fs_getServerSequenceNumber();
    }
else if( $action == "report_death" ) {
    fs_reportDeath();
    }
else if( $action == "get_score" ) {
    fs_getScore();
    }
else if( $action == "get_client_score" ) {
    fs_getClientScore();
    }
else if( $action == "get_client_score_details" ) {
    fs_getClientScoreDetails();
    }
else if( $action == "show_log" ) {
    fs_showLog();
    }
else if( $action == "clear_log" ) {
    fs_clearLog();
    }
else if( $action == "show_data" ) {
    fs_showData();
    }
else if( $action == "show_detail" ) {
    fs_showDetail();
    }
else if( $action == "reset_scores" ) {
    fs_resetScores();
    }
else if( $action == "logout" ) {
    fs_logout();
    }
else if( $action == "show_leaderboard" ) {
    fs_showLeaderboard();
    }
else if( $action == "leaderboard_detail" ) {
    fs_leaderboardDetail();
    }
else if( $action == "fs_setup" ) {
    global $setup_header, $setup_footer;
    echo $setup_header; 

    echo "<H2>Fitness Server Web-based Setup</H2>";

    echo "Creating tables:<BR>";

    echo "<CENTER><TABLE BORDER=0 CELLSPACING=0 CELLPADDING=1>
          <TR><TD BGCOLOR=#000000>
          <TABLE BORDER=0 CELLSPACING=0 CELLPADDING=5>
          <TR><TD BGCOLOR=#FFFFFF>";

    fs_setupDatabase();

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
        fs_doesTableExist( $tableNamePrefix . "users" ) &&
        fs_doesTableExist( $tableNamePrefix . "lives" ) &&
        fs_doesTableExist( $tableNamePrefix . "offspring" ) &&
        fs_doesTableExist( $tableNamePrefix . "first_names" ) &&
        fs_doesTableExist( $tableNamePrefix . "last_names" ) &&
        fs_doesTableExist( $tableNamePrefix . "log" );
    
        
    if( $exists  ) {
        echo "Fitness Server database setup and ready";
        }
    else {
        // start the setup procedure

        global $setup_header, $setup_footer;
        echo $setup_header; 

        echo "<H2>Fitness Server Web-based Setup</H2>";
    
        echo "Fitness Server will walk you through a " .
            "brief setup process.<BR><BR>";
        
        echo "Step 1: ".
            "<A HREF=\"server.php?action=fs_setup\">".
            "create the database tables</A>";

        echo $setup_footer;
        }
    }



// done processing
// only function declarations below

fs_closeDatabase();




function fs_populateNameTable( $inTableName, $inFileName ) {

    if( $file = fopen( $inFileName, "r" ) ) {

        $firstLine = true;

        $query = "INSERT INTO $inTableName ( name ) VALUES ";

        while( !feof( $file ) ) {
            $line = trim( fgets( $file) );

            if( $line == "" ) {
                continue;
                }
            
            if( ! $firstLine ) {
                $query = $query . ",";
                }
                
            $query = $query . " ( '$line' )";
                
            $firstLine = false;
            }
        
        fclose( $file );

        $query = $query . ";";

        $result = fs_queryDatabase( $query );
        }
    else {
        echo "(Failed to populate, couldn't open file $inFileName)<br>";
        }
    }




/**
 * Creates the database tables needed by seedBlogs.
 */
function fs_setupDatabase() {
    global $tableNamePrefix;

    $tableName = $tableNamePrefix . "log";
    if( ! fs_doesTableExist( $tableName ) ) {

        // this table contains general info about the server
        // use INNODB engine so table can be locked
        $query =
            "CREATE TABLE $tableName(" .
            "entry TEXT NOT NULL, ".
            "entry_time DATETIME NOT NULL, ".
            "index( entry_time ) );";

        $result = fs_queryDatabase( $query );

        echo "<B>$tableName</B> table created<BR>";
        }
    else {
        echo "<B>$tableName</B> table already exists<BR>";
        }


    
    $tableName = $tableNamePrefix . "servers";
    if( ! fs_doesTableExist( $tableName ) ) {

        $query =
            "CREATE TABLE $tableName(" .
            "id INT UNSIGNED NOT NULL PRIMARY KEY AUTO_INCREMENT," .
            // example:  server1.onehouronelife.com
            "name VARCHAR(254) NOT NULL," .
            "UNIQUE KEY( name ),".
            // for use with server requests
            "sequence_number INT NOT NULL );";

        $result = fs_queryDatabase( $query );

        echo "<B>$tableName</B> table created<BR>";
        }
    else {
        echo "<B>$tableName</B> table already exists<BR>";
        }


    
    
    $tableName = $tableNamePrefix . "users";
    if( ! fs_doesTableExist( $tableName ) ) {

        $query =
            "CREATE TABLE $tableName(" .
            "id INT UNSIGNED NOT NULL PRIMARY KEY AUTO_INCREMENT," .
            "email VARCHAR(254) NOT NULL," .
            "UNIQUE KEY( email )," .
            "leaderboard_name varchar(254) NOT NULL,".
            "lives_affecting_score INT UNSIGNED NOT NULL,".
            "score FLOAT NOT NULL," .
            "index( score )," .
            "last_action_time DATETIME NOT NULL,".
            "index( last_action_time, score ),".
            // for use with client connections
            "client_sequence_number INT NOT NULL );";

        $result = fs_queryDatabase( $query );

        echo "<B>$tableName</B> table created<BR>";
        }
    else {
        echo "<B>$tableName</B> table already exists<BR>";
        }



    // holds common info about each life, referenced from offspring table
    $tableName = $tableNamePrefix . "lives";
    if( ! fs_doesTableExist( $tableName ) ) {

        $query =
            "CREATE TABLE $tableName(" .
            "id INT UNSIGNED NOT NULL PRIMARY KEY AUTO_INCREMENT," .
            "life_player_id INT UNSIGNED NOT NULL,".
            "name VARCHAR(254) NOT NULL,".
            // age at time of death in years
            "age FLOAT UNSIGNED NOT NULL,".
            "display_id INT UNSIGNED NOT NULL );";

        $result = fs_queryDatabase( $query );

        echo "<B>$tableName</B> table created<BR>";
        }
    else {
        echo "<B>$tableName</B> table already exists<BR>";
        }

    
    $tableName = $tableNamePrefix . "offspring";
    if( ! fs_doesTableExist( $tableName ) ) {

        $query =
            "CREATE TABLE $tableName(" .
            "player_id INT UNSIGNED NOT NULL," .
            "life_id INT UNSIGNED NOT NULL,".
            "PRIMARY KEY( player_id, life_id ),".
            // this will be You for self
            // or Granddaughter, etc.
            "relation_name VARCHAR(254) NOT NULL,".
            // player_id's score before this offspring
            "old_score FLOAT NOT NULL,".
            // player_id's score after this offspring
            "new_score FLOAT NOT NULL,".
            "death_time DATETIME NOT NULL,".
            // for fast filtering/sorting of most-recent 20
            // offspring for a given player
            "index( player_id, death_time ) );";

        $result = fs_queryDatabase( $query );

        echo "<B>$tableName</B> table created<BR>";
        }
    else {
        echo "<B>$tableName</B> table already exists<BR>";
        }


    
    $tableName = $tableNamePrefix . "first_names";
    if( ! fs_doesTableExist( $tableName ) ) {

        $query =
            "CREATE TABLE $tableName(" .
            "id INT UNSIGNED NOT NULL PRIMARY KEY AUTO_INCREMENT," .
            "name VARCHAR(245) NOT NULL );";

        $result = fs_queryDatabase( $query );

        echo "<B>$tableName</B> table created<BR>";

        fs_populateNameTable( $tableName, "firstNames.txt" );
        }
    else {
        echo "<B>$tableName</B> table already exists<BR>";
        }


    
    $tableName = $tableNamePrefix . "last_names";
    if( ! fs_doesTableExist( $tableName ) ) {

        $query =
            "CREATE TABLE $tableName(" .
            "id INT UNSIGNED NOT NULL PRIMARY KEY AUTO_INCREMENT," .
            "name VARCHAR(245) NOT NULL );";

        $result = fs_queryDatabase( $query );

        echo "<B>$tableName</B> table created<BR>";

        fs_populateNameTable( $tableName, "lastNames.txt" );
        }
    else {
        echo "<B>$tableName</B> table already exists<BR>";
        }

    
    }



function fs_showLog() {
    fs_checkPassword( "show_log" );

     echo "[<a href=\"server.php?action=show_data" .
         "\">Main</a>]<br><br><br>";
    
    global $tableNamePrefix;

    $query = "SELECT * FROM $tableNamePrefix"."log ".
        "ORDER BY entry_time DESC;";
    $result = fs_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );



    echo "<a href=\"server.php?action=clear_log\">".
        "Clear log</a>";
        
    echo "<hr>";
        
    echo "$numRows log entries:<br><br><br>\n";
        

    for( $i=0; $i<$numRows; $i++ ) {
        $time = fs_mysqli_result( $result, $i, "entry_time" );
        $entry = htmlspecialchars( fs_mysqli_result( $result, $i, "entry" ) );

        echo "<b>$time</b>:<br>$entry<hr>\n";
        }
    }



function fs_clearLog() {
    fs_checkPassword( "clear_log" );

     echo "[<a href=\"server.php?action=show_data" .
         "\">Main</a>]<br><br><br>";
    
    global $tableNamePrefix;

    $query = "DELETE FROM $tableNamePrefix"."log;";
    $result = fs_queryDatabase( $query );
    
    if( $result ) {
        echo "Log cleared.";
        }
    else {
        echo "DELETE operation failed?";
        }
    }




















function fs_logout() {
    fs_checkReferrer();
    
    fs_clearPasswordCookie();

    echo "Logged out";
    }




function fs_showData( $checkPassword = true ) {
    // these are global so they work in embeded function call below
    global $skip, $search, $order_by;

    if( $checkPassword ) {
        fs_checkPassword( "show_data" );
        }
    
    global $tableNamePrefix, $remoteIP;
    

    echo "<table width='100%' border=0><tr>".
        "<td>[<a href=\"server.php?action=show_data" .
            "\">Main</a>]</td>".
        "<td align=right>[<a href=\"server.php?action=logout" .
            "\">Logout</a>]</td>".
        "</tr></table><br><br><br>";




    $skip = fs_requestFilter( "skip", "/[0-9]+/", 0 );
    
    global $usersPerPage;
    
    $search = fs_requestFilter( "search", "/[A-Z0-9_@. \-]+/i" );

    $order_by = fs_requestFilter( "order_by", "/[A-Z_]+/i",
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

    $result = fs_queryDatabase( $query );
    $totalRecords = fs_mysqli_result( $result, 0, 0 );


    $orderDir = "DESC";

    if( $order_by == "email" ) {
        $orderDir = "ASC";
        }
    
             
    $query = "SELECT * ".
        "FROM $tableNamePrefix"."users $keywordClause".
        "ORDER BY $order_by $orderDir ".
        "LIMIT $skip, $usersPerPage;";
    $result = fs_queryDatabase( $query );
    
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
    echo "<td>".orderLink( "leaderboard_name", "Leaderbord Name" )."</td>\n";
    echo "<td>".orderLink( "last_action_time", "Last Action" )."</td>\n";
    echo "<td>".orderLink( "score", "Score" )."</td>\n";
    echo "<td>".orderLink( "lives_affecting_score", "Lives Counted" )."</td>\n";
    echo "</tr>\n";


    for( $i=0; $i<$numRows; $i++ ) {
        $id = fs_mysqli_result( $result, $i, "id" );
        $email = fs_mysqli_result( $result, $i, "email" );
        $leaderboard_name = fs_mysqli_result( $result, $i, "leaderboard_name" );
        $last_action_time = fs_mysqli_result( $result, $i, "last_action_time" );
        $score = fs_mysqli_result( $result, $i, "score" );
        $lives_affecting_score =
            fs_mysqli_result( $result, $i, "lives_affecting_score" );
        
        $encodedEmail = urlencode( $email );

        
        echo "<tr>\n";

        echo "<td>$id</td>\n";
        echo "<td>".
            "<a href=\"server.php?action=show_detail&email=$encodedEmail\">".
            "$email</a></td>\n";
        echo "<td>$leaderboard_name</td>\n";
        echo "<td>$last_action_time</td>\n";
        echo "<td>$score</td>\n";
        echo "<td>$lives_affecting_score</td>\n";
        echo "</tr>\n";
        }
    echo "</table>";


    echo "<hr>";

    global $startingScore;
?>
    <FORM ACTION="server.php" METHOD="post">
         New Score: 
    <INPUT TYPE="hidden" NAME="action" VALUE="reset_scores">
    <INPUT TYPE="text" MAXLENGTH=10 SIZE=5 NAME="target_score"
           VALUE="<?php echo $startingScore;?>">

    <INPUT TYPE="checkbox" NAME="confirm" VALUE=1> Confirm  
    <INPUT TYPE="Submit" VALUE="Reset All Scores">
    </FORM>
<?php

    echo "<hr>";
         
    echo "<a href=\"server.php?action=show_log\">".
        "Show log</a>";
    echo "<hr>";
    echo "Generated for $remoteIP\n";

    }








function fs_showDetail( $checkPassword = true ) {
    if( $checkPassword ) {
        fs_checkPassword( "show_detail" );
        }
    
    echo "[<a href=\"server.php?action=show_data" .
         "\">Main</a>]<br><br><br>";
    
    global $tableNamePrefix;
    

    // two possible params... id or email

    $id = fs_requestFilter( "id", "/[0-9]+/i", -1 );
    $email = "";

    
    if( $id != -1 ) {
        $query = "SELECT email ".
            "FROM $tableNamePrefix"."users ".
            "WHERE id = '$id';";
        $result = fs_queryDatabase( $query );
        
        $email = fs_mysqli_result( $result, 0, "email" );
        }
    else {
        $email = fs_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i" );
    
        $query = "SELECT id ".
            "FROM $tableNamePrefix"."users ".
            "WHERE email = '$email';";
        $result = fs_queryDatabase( $query );
        
        $id = fs_mysqli_result( $result, 0, "id" );
        }

    
    $aveAge = fs_getAveAge( $email );
    

    
    $query = "SELECT name, age, relation_name, ".
        "old_score, new_score, death_time, life_player_id ".
        "FROM $tableNamePrefix"."offspring AS offspring ".
        "INNER JOIN $tableNamePrefix"."lives AS lives ".
        "ON offspring.life_id = lives.id ".
        "WHERE offspring.player_id = $id ORDER BY offspring.death_time DESC";

    
    $result = fs_queryDatabase( $query );

    echo "<center><table border=0><tr><td>";
    
    echo "<b>ID:</b> $id<br><br>";
    echo "<b>Email:</b> $email<br><br>";
    echo "<font color=green><b>Ave Age:</b> $aveAge</font><br><br>";
    echo "</td></tr></table>";

    $numRows = mysqli_num_rows( $result );

    global $numLivesInAverage;
    $numYouLives = 0;
    
    echo "<table border=1 cellpadding=10 cellspacing=0>";
    for( $i=0; $i<$numRows; $i++ ) {
        $name = fs_mysqli_result( $result, $i, "name" );
        $age = fs_mysqli_result( $result, $i, "age" );
        $relation_name = fs_mysqli_result( $result, $i, "relation_name" );
        $old_score = fs_mysqli_result( $result, $i, "old_score" );
        $new_score = fs_mysqli_result( $result, $i, "new_score" );
        $death_time = fs_mysqli_result( $result, $i, "death_time" );

        $otherLifePlayerID =
            fs_mysqli_result( $result, $i, "life_player_id" );

        
        $delta = round( $new_score - $old_score, 3 );

        $deltaString;

        if( $delta < 0 ) {
            $deltaString = " - " . abs( $delta );
            }
        else {
            $deltaString = " + " . $delta;
            }
        
        echo "<tr>";

        if( $relation_name != "You" ) {
            // link to other player
            echo "<td><a href='server.php?".
                "action=show_detail&id=$otherLifePlayerID'>$name</a></td>";
            }
        else {
            echo "<td>$name</td>";
            }
        
        if( $old_score != $new_score &&
            $relation_name == "You" &&
            $numYouLives < $numLivesInAverage ) {
            echo "<td><font color=green><b>$age years old</b></font></td>";
            $numYouLives ++;
            }
        else {
            echo "<td>$age years old</td>";
            }
        echo "<td>$relation_name</td>";
        echo "<td>$old_score</td>";
        echo "<td>$deltaString</td>";
        echo "<td>$new_score</td>";
        echo "<td>$death_time</td>";
        }
    echo "</table></center>";
    }


function fs_resetScores( $checkPassword = true ) {
    if( $checkPassword ) {
        fs_checkPassword( "reset_scores" );
        }
    
    echo "[<a href=\"server.php?action=show_data" .
         "\">Main</a>]<br><br><br>";
    
    global $tableNamePrefix;

    $confirm = fs_requestFilter( "confirm", "/[01]/i", 0 );

    if( $confirm == 0 ) {
        echo "Must confirm";
        return;
        }
    $targetScore = fs_requestFilter( "target_score", "/[0-9\-]+/i", -999 );

    if( $targetScore == -999 ) {
        echo "Invalid target score";
        return;
        }

    $query = "INSERT INTO $tableNamePrefix"."lives ".
        "SET name = 'Score_Reset', age = 42.0, display_id =3201, ".
        "life_player_id=-1";

    fs_queryDatabase( $query );
    
    global $fs_mysqlLink;
    $life_id = mysqli_insert_id( $fs_mysqlLink );

    
    $query = "SELECT id, score ".
        "FROM $tableNamePrefix"."users ".
        "WHERE score != $targetScore;";

    $result = fs_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );
    
    for( $i=0; $i<$numRows; $i++ ) {
        
        $id = fs_mysqli_result( $result, $i, "id" );
        $score = fs_mysqli_result( $result, $i, "score" );

        $query = "INSERT into $tableNamePrefix"."offspring ".
            "SET player_id = $id, life_id = $life_id, ".
            "relation_name = 'Affects_Everyone', ".
            "old_score = $score, ".
            "new_score = $targetScore, ".
            "death_time = CURRENT_TIMESTAMP; ";
        fs_queryDatabase( $query );
        
        $query = "UPDATE $tableNamePrefix"."users ".
            "SET score = $targetScore WHERE id = $id;";
        fs_queryDatabase( $query );
        }

    echo "<br>Processed $numRows users<br><br>";
    }



function fs_showLeaderboard() {
    
    global $tableNamePrefix;

    global $header, $footer;

    eval( $header );

    echo "<center>";

    global $leaderboardHours;
    
    $query = "SELECT id, leaderboard_name, score ".
        "FROM $tableNamePrefix"."users ".
        "WHERE last_action_time > ".
        "   DATE_SUB( NOW(), INTERVAL $leaderboardHours HOUR )".
        "ORDER BY score DESC limit 1000;";

    $result = fs_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );
    
    echo "<table border=0 cellpadding=20>";

    for( $i=0; $i<$numRows; $i++ ) {
        $place = $i + 1;
        
        $id = fs_mysqli_result( $result, $i, "id" );
        $name = fs_mysqli_result( $result, $i, "leaderboard_name" );
        $score = fs_mysqli_result( $result, $i, "score" );

        echo "<tr><td>$place.</td>".
            "<td>".
            "<a href=\"server.php?action=leaderboard_detail&id=$id\">".
            "$name</a></td><td>$score</td></tr>";
        }
    echo "</table>";
    
        
    echo "</center>";

    eval( $footer );
    }





function fs_leaderboardDetail() {
    
    global $tableNamePrefix;

    global $header, $footer;

    eval( $header );

    echo "<center>";

    
    $id = fs_requestFilter( "id", "/[0-9]+/", 0 );

    
    $query = "SELECT email, leaderboard_name ".
        "FROM $tableNamePrefix"."users ".
        "WHERE id = $id;";
    $result = fs_queryDatabase( $query );

    $email = fs_mysqli_result( $result, 0, "email" );
    $leaderboard_name = fs_mysqli_result( $result, 0, "leaderboard_name" );

    $aveAge = fs_getAveAge( $email );

    $aveAge = round( $aveAge, 3 );
    
    $query = "SELECT life_id, life_player_id, name, age, relation_name, ".
        "old_score, new_score, death_time ".
        "FROM $tableNamePrefix"."offspring AS offspring ".
        "INNER JOIN $tableNamePrefix"."lives AS lives ".
        "ON offspring.life_id = lives.id ".
        "WHERE offspring.player_id = $id ORDER BY offspring.death_time DESC";

    
    $result = fs_queryDatabase( $query );

    echo "<center><table border=0><tr>";
    
    echo "<td><b>Leaderboard Name:</b></td><td>$leaderboard_name</td></tr>";
    echo "<td><font color=green><b>Ave Age:</b></font></td>".
        "<td><font color=green>$aveAge</font>";
    echo "</td></tr></table>";

    $numRows = mysqli_num_rows( $result );

    global $numLivesInAverage;
    $numYouLives = 0;
    
    echo "<table border=1 cellpadding=10 cellspacing=0>";
    for( $i=0; $i<$numRows; $i++ ) {
        $name = fs_mysqli_result( $result, $i, "name" );
        $age = fs_mysqli_result( $result, $i, "age" );
        $relation_name = fs_mysqli_result( $result, $i, "relation_name" );
        $old_score = fs_mysqli_result( $result, $i, "old_score" );
        $new_score = fs_mysqli_result( $result, $i, "new_score" );
        $death_time = fs_mysqli_result( $result, $i, "death_time" );


        $life_id = fs_mysqli_result( $result, $i, "life_id" );

        $life_player_id = fs_mysqli_result( $result, $i, "life_player_id" );

        
        $deathAgoSec = strtotime( "now" ) - strtotime( $death_time );
        
        $deathAgo = fs_secondsToAgeSummary( $deathAgoSec );

        
        $delta = round( $new_score - $old_score, 3 );

        $deltaString;

        if( $delta < 0 ) {
            $deltaString = " - " . abs( $delta );
            }
        else {
            $deltaString = " + " . $delta;
            }

        $old_score = round( $old_score, 3 );
        $new_score = round( $new_score, 3 );
        
        echo "<tr>";

        $name = str_replace( "_", " ", $name );


        if( $life_player_id == -1 ||
            $relation_name == "You" ||
            $relation_name == "Affects_Everyone" ) {
            echo "<td>$name</td>";
            }
        else {
            // link to other player who lived this life
            

            if( $life_player_id == 0 ) {
                $life_player_id = -1;
                
                // life_player_id hasn't been set yet
                // this must be an older entry
                // see if we can set it now
                
                $query = "SELECT player_id FROM $tableNamePrefix"."offspring ".
                    "WHERE life_id = $life_id AND ".
                    "relation_name = 'You';";
                
                $resultSub = fs_queryDatabase( $query );
                
                if( mysqli_num_rows( $resultSub ) == 1 ) {
                    
                    $life_player_id =
                        fs_mysqli_result( $resultSub, 0, "player_id" );
                    }
                
                // otherwise, set it to -1 so we don't have to try
                // settting it again later

                $query = "UPDATE $tableNamePrefix"."lives ".
                    "SET life_player_id = $life_player_id ".
                    "WHERE id = $life_id;";
                fs_queryDatabase( $query );
                }
            
            if( $life_player_id > 0 ) {
                echo "<td>".
                    "<a href=\"server.php?".
                    "action=leaderboard_detail&id=$life_player_id\">".
                    "$name</a></td>";
                }
            else {
                echo "<td>$name</td>";
                }
            }
        
        $age = round( $age, 3 );
        
        if( $old_score != $new_score &&
            $relation_name == "You" &&
            $numYouLives < $numLivesInAverage ) {
            echo "<td><font color=green><b>$age years old</b></font></td>";
            $numYouLives ++;
            }
        else {
            echo "<td>$age years old</td>";
            }

        $relation_name = str_replace( "_", " ", $relation_name );
        
        echo "<td>$relation_name</td>";
        echo "<td>$old_score</td>";
        echo "<td nowrap='nowrap'>$deltaString</td>";
        echo "<td>$new_score</td>";
        echo "<td>$deathAgo ago</td>";
        }
    echo "</table></center>";

    eval( $footer );
    }








function fs_getClientSequenceNumber() {
    global $tableNamePrefix;
    

    $email = fs_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i", "" );

    if( $email == "" ) {
        $rawEmail = $_REQUEST[ "email" ];
        fs_log( "getClientSequenceNumber denied for bad email '$rawEmail'" );

        echo "DENIED";
        return;
        }
    
    
    $seq = fs_getClientSequenceNumberForEmail( $email );

    echo "$seq\n"."OK";
    }



// assumes already-filtered, valid email
// returns 0 if not found
function fs_getClientSequenceNumberForEmail( $inEmail ) {
    global $tableNamePrefix;
    
    $query = "SELECT client_sequence_number FROM $tableNamePrefix"."users ".
        "WHERE email = '$inEmail';";
    $result = fs_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );

    if( $numRows < 1 ) {
        return 0;
        }
    else {
        return fs_mysqli_result( $result, 0, "client_sequence_number" );
        }
    }





function fs_getServerSequenceNumber() {
    global $tableNamePrefix;
    

    $name = fs_requestFilter( "server_name", "/[A-Z0-9.\-]+/i", "" );

    if( $name == "" ) {
        fs_log( "getServerSequenceNumber denied for bad name" );

        echo "DENIED";
        return;
        }
    
    
    $seq = fs_getServerSequenceNumberForName( $name );

    echo "$seq\n"."OK";
    }



// assumes already-filtered, valid name
// returns 0 if not found
function fs_getServerSequenceNumberForName( $inName ) {
    global $tableNamePrefix;
    
    $query = "SELECT sequence_number FROM $tableNamePrefix"."servers ".
        "WHERE name = '$inName';";
    $result = fs_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );

    if( $numRows < 1 ) {
        return 0;
        }
    else {
        return fs_mysqli_result( $result, 0, "sequence_number" );
        }
    }






function fs_checkServerSeqHash( $name ) {
    global $sharedGameServerSecret;


    global $action;


    $sequence_number = fs_requestFilter( "sequence_number", "/[0-9]+/i", "0" );

    $hash_value = fs_requestFilter( "hash_value", "/[A-F0-9]+/i", "" );

    $hash_value = strtoupper( $hash_value );


    if( $name == "" ) {

        fs_log( "checkServerSeqHash denied for bad server name" );
        
        echo "DENIED";
        die();
        }
    
    $trueSeq = fs_getServerSequenceNumberForName( $name );

    if( $trueSeq > $sequence_number ) {
        global $action;
        
        fs_log( "checkServerSeqHash denied for stale sequence number ".
                "$sequence_number from ".
                "server $name (action=$action) (trueSeq=$trueSeq)");

        echo "DENIED";
        die();
        }

    $computedHashValue =
        strtoupper( fs_hmac_sha1( $sharedGameServerSecret, $sequence_number ) );

    if( $computedHashValue != $hash_value ) {
        // fs_log( "curse denied for bad hash value" );

        echo "DENIED";
        die();
        }

    return $trueSeq;
    }




function fs_checkClientSeqHash( $email ) {
    global $sharedGameServerSecret;


    global $action;


    $sequence_number = fs_requestFilter( "sequence_number", "/[0-9]+/i", "0" );

    $hash_value = fs_requestFilter( "hash_value", "/[A-F0-9]+/i", "" );

    $hash_value = strtoupper( $hash_value );


    if( $email == "" ) {
        $rawEmail = $_REQUEST[ "email" ];

        fs_log( "checkClientSeqHash denied for bad email '$rawEmail'" );
        
        echo "DENIED";
        die();
        }
    
    $trueSeq = fs_getClientSequenceNumberForEmail( $email );

    if( $trueSeq > $sequence_number ) {
        fs_log( "checkClientSeqHash denied for stale sequence number ".
                "($email submitted $sequence_number trueSeq=$trueSeq" );

        echo "DENIED";
        die();
        }

    $correct = false;

    $encodedEmail = urlencode( $email );

    
    global $ticketServerURL;
    $url = "$ticketServerURL".
        "?action=check_ticket_hash".
        "&email=$encodedEmail".
        "&hash_value=$hash_value".
        "&string_to_hash=$sequence_number";

    
    $result = trim( file_get_contents( $url ) );
            
    if( $result == "VALID" ) {
        $correct = true;
        }

    
    if( ! $correct ) {
        fs_log( "checkClientSeqHash denied, hash check failed" );

        echo "DENIED";
        die();
        }

    
    return $trueSeq;
    }




function fs_pickLeaderboardName( $inEmail ) {
    global $tableNamePrefix;

    $query = "SELECT COUNT(*) FROM $tableNamePrefix"."first_names ;";

    $result = fs_queryDatabase( $query );
    $firstCount = fs_mysqli_result( $result, 0, 0 );

    $query = "SELECT COUNT(*) FROM $tableNamePrefix"."last_names ;";

    $result = fs_queryDatabase( $query );
    $lastCount = fs_mysqli_result( $result, 0, 0 );

    
    // include secret in hash, so people with code can't match
    // names to emails
    global $sharedGameServerSecret;
    
    $emailHash = sha1( $inEmail . $sharedGameServerSecret );

    $seedA = hexdec( substr( $emailHash, 0, 8 ) );
    $seedB = hexdec( substr( $emailHash, 8, 8 ) );
    
    mt_srand( $seedA );

    $firstPick = mt_rand( 1, $firstCount );

    mt_srand( $seedB );
    $lastPick = mt_rand( 1, $lastCount );


    $query = "SELECT name FROM $tableNamePrefix"."first_names ".
        "WHERE id = $firstPick;";

    $result = fs_queryDatabase( $query );
    $firstName = fs_mysqli_result( $result, 0, 0 );


    $query = "SELECT name FROM $tableNamePrefix"."last_names ".
        "WHERE id = $lastPick;";

    $result = fs_queryDatabase( $query );
    $lastName = fs_mysqli_result( $result, 0, 0 );

    $query = "SELECT COUNT(*) FROM $tableNamePrefix"."last_names ;";


    return ucwords( strtolower( "$firstName $lastName" ) );
    }




function sign( $n ) {
    return ( $n > 0 ) - ( $n < 0 );
    }



function fs_getUserID( $inEmail ) {
    global $tableNamePrefix;

    $query = "SELECT COUNT(*) FROM $tableNamePrefix"."users ".
        "WHERE email = '$inEmail';";

    $result = fs_queryDatabase( $query );
    $count = fs_mysqli_result( $result, 0, 0 );

    if( $count == 0 ) {
        fs_addUserRecord( $inEmail );
        }

    $query = "SELECT id, score FROM $tableNamePrefix"."users ".
        "WHERE email = '$inEmail';";

    $result = fs_queryDatabase( $query );
    return fs_mysqli_result( $result, 0, "id" );
    }



// log a death that will affect the score of $inEmail
// if $inNoScore is true, the relationship is still logged, but the
// score change is fixed at 0 AND the last_action_time isn't updated
// (to prevent noScore lives from bringing an expired player back to the
//  leaderboard)
function fs_logDeath( $inEmail, $life_id, $inRelName, $inAge,
                      $inDeadPersonScore,
                      $inNoScore = false ) {
    global $tableNamePrefix;

    $query = "SELECT COUNT(*) FROM $tableNamePrefix"."users ".
        "WHERE email = '$inEmail';";

    $result = fs_queryDatabase( $query );
    $count = fs_mysqli_result( $result, 0, 0 );

    if( $count == 0 ) {
        fs_addUserRecord( $inEmail );
        }

    $query = "SELECT id, score FROM $tableNamePrefix"."users ".
        "WHERE email = '$inEmail';";

    $result = fs_queryDatabase( $query );
    $player_id = fs_mysqli_result( $result, 0, "id" );
    $old_score = fs_mysqli_result( $result, 0, "score" );


    $query = "SELECT COUNT(*) from $tableNamePrefix"."offspring ".
        "WHERE player_id = $player_id AND life_id = $life_id;";
    $result = fs_queryDatabase( $query );

    $count = fs_mysqli_result( $result, 0, 0 );

    if( $count > 0 ) {
        // this life has already been counted toward the score of this
        // email address
        // (perhaps this life was our daughter in a previous life
        //  and our mother in this life---we've already counted the score
        //  from this life as our daughter.)
        // Don't double-count.
        return;
        }
    
    
    // score update
    global $formulaR, $formulaK;


    $delta = $inAge - $inDeadPersonScore;


    // wondible's suggestion:

    // Map this delta, which is in their score range,
    // into the range of OUR score

    // note that if we ARE the dead person (updating our own score)
    // this operation should have no effect, because $inDeadPersonScore
    // will be equal to old_score
    
    $mappedDelta = 0;

    if( $inAge < $inDeadPersonScore ) {
        // they lived shorter than average
        $fraction = $inAge / $inDeadPersonScore;

        $mappedAge = $fraction * $old_score;

        $mappedDelta = $mappedAge - $old_score;
        }
    else if( $inAge > $inDeadPersonScore ) {
        // here we have see how far they are away from the cap
        // and map that a similar gap-closing between our score and the
        // cap
        $cap = 60;
        
        $gap = $cap - $inDeadPersonScore;

        $gapFraction = $delta / $gap;

        $ourGap = $cap - $old_score;

        $mappedDelta = $ourGap * $gapFraction;
        }


    // now apply formula to the mapped delta value
    $delta = $mappedDelta;
    
    
    if( $formulaR != 1 ) {
        // preserve sign after power operation
        $s = sign( $delta );

        // remove any negative
        $delta *= $s;
        
        $delta = pow( $delta, $formulaR );

        // restore any negative
        $delta *= $s;
        }
    $delta /= $formulaK;

    if( $inNoScore ) {
        $delta = 0;
        }
    
    $new_score = $old_score + $delta;
    
    
    
    $query = "INSERT into $tableNamePrefix"."offspring ".
        "SET player_id = $player_id, life_id = $life_id, ".
        "relation_name = '$inRelName', old_score = $old_score,".
        "new_score = $new_score, death_time = CURRENT_TIMESTAMP;";

    fs_queryDatabase( $query );


    $timeUpdateClause = "";

    if( ! $inNoScore ) {
        $timeUpdateClause = ", last_action_time = CURRENT_TIMESTAMP";
        }
    
    $query = "UPDATE $tableNamePrefix"."users ".
        "SET lives_affecting_score = lives_affecting_score + 1, ".
        "score = $new_score $timeUpdateClause ".
        "WHERE email = '$inEmail';";
    
    fs_queryDatabase( $query );
    
    }




function fs_checkAndUpdateServerSeqNumber() {
    global $tableNamePrefix;
    
    $server_name = fs_requestFilter( "server_name", "/[A-Z0-9.\-]+/i", "" );
    
    $trueSeq = fs_checkServerSeqHash( $server_name );
    
    
    // no locking is done here, because action is asynchronous anyway

    if( $trueSeq == 0 ) {
        // no record exists, add one
        $query = "INSERT INTO $tableNamePrefix". "servers SET " .
            "name = '$server_name', ".
            "sequence_number = 1 ".
            "ON DUPLICATE KEY UPDATE sequence_number = sequence_number + 1;";
        fs_queryDatabase( $query );
        }
    else {
        $query = "UPDATE $tableNamePrefix". "servers SET " .
            "sequence_number = sequence_number + 1 ".
            "WHERE name = '$server_name';";
        fs_queryDatabase( $query );
        }
    }



function fs_checkAndUpdateClientSeqNumber() {
    global $tableNamePrefix;

    $email = fs_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i", "" );

    $trueSeq = fs_checkClientSeqHash( $email );
    
    
    // no locking is done here, because action is asynchronous anyway

    if( $trueSeq == 0 ) {
        // no record exists, add one
        fs_addUserRecord( $email );
        }
    else {
        $query = "UPDATE $tableNamePrefix". "users SET " .
            "client_sequence_number = client_sequence_number + 1 ".
            "WHERE email = '$email';";
        fs_queryDatabase( $query );
        }
    }




function fs_addUserRecord( $inEmail ) {
    $leaderboard_name = fs_pickLeaderboardName( $inEmail );

    global $tableNamePrefix, $startingScore;
    
    $query = "INSERT INTO $tableNamePrefix"."users ".
        "SET email = '$inEmail', leaderboard_name = '$leaderboard_name', ".
        "lives_affecting_score = 0, score=$startingScore, ".
        "last_action_time=CURRENT_TIMESTAMP, client_sequence_number=1;";
    
    fs_queryDatabase( $query );
    }



function fs_cleanLifeType( $player_id, $relationNameClause, $numToKeep,
                           $removedOffspring ) {
    global $tableNamePrefix;
    
    $query = "SELECT life_id ".
        "FROM $tableNamePrefix"."offspring ".
        "WHERE player_id = $player_id AND $relationNameClause ".
        "ORDER BY death_time DESC ".
        "LIMIT $numToKeep,9999999";

    $result = fs_queryDatabase( $query );
    
    $numRows = mysqli_num_rows( $result );

    $numOffspringRemoved = 0;
    
    $removedOffspring = array();
    for( $i=0; $i<$numRows; $i++ ) {
        $life_id = fs_mysqli_result( $result, $i, "life_id" );

        $removedOffspring[] = $life_id;
        
        $query = "DELETE FROM $tableNamePrefix"."offspring ".
            "WHERE player_id = $player_id AND life_id = $life_id;";
        fs_queryDatabase( $query );
        $numOffspringRemoved ++;
        }
    }



// cleans old offspring from table that count for $email
// and deletes the lives themselves if they aren't in the offspring table
// for anyone anymore
function fs_cleanOldLives( $email ) {
    global $tableNamePrefix, $maxOffspringHistoryToKeep, $numLivesInAverage;

    $query = "SELECT id FROM $tableNamePrefix"."users ".
        "WHERE email = '$email';";

    $result = fs_queryDatabase( $query );
    
    if( mysqli_num_rows( $result ) == 0 ) {
        return;
        }
    
    $player_id = fs_mysqli_result( $result, 0, "id" );

    $removedOffspring = array();
    
    // skip $maxOffspringHistoryToKeep for non-You lives,
    // and delete all non-You records after that
    fs_cleanLifeType( $player_id,
                      " relation_name != 'You' ",
                      $maxOffspringHistoryToKeep,
                      $removedOffspring );

    // skip $numLivesInAverage for You lives,
    // and delete all You records after that
    fs_cleanLifeType( $player_id,
                      " relation_name = 'You' ",
                      $numLivesInAverage,
                      $removedOffspring );

    
    // now we need to check offspring table and see if any of these
    // life_ids aren't anyone's offspring anymore
    $numLivesRemoved = 0;
    
    foreach( $removedOffspring as $life_id ) {
        $query = "SELECT COUNT(*) FROM $tableNamePrefix"."offspring ".
            "WHERE life_id = $life_id;";

        $result = fs_queryDatabase( $query );

        $count = fs_mysqli_result( $result, 0, 0 );

        if( $count == 0 ) {
            // no one counting this life as an offspring anymore
            $query = "DELETE FROM $tableNamePrefix"."lives ".
                "WHERE id = $life_id;";
            fs_queryDatabase( $query );

            $numLivesRemoved ++;
            }
        }
    }





function fs_getPlayerScore( $inEmail ) {
    global $startingScore, $tableNamePrefix;
    
    $query = "SELECT score FROM $tableNamePrefix"."users ".
        "WHERE email = '$inEmail';";

    $result = fs_queryDatabase( $query );

    $score = $startingScore;

    if( mysqli_num_rows( $result ) > 0 ) {    
        $score = fs_mysqli_result( $result, 0, "score" );
        }

    return $score;
    }



function fs_getAveAge( $inEmail ) {
    global $startingScore, $tableNamePrefix;
    
    $query = "SELECT id FROM $tableNamePrefix"."users ".
        "WHERE email = '$inEmail';";

    $result = fs_queryDatabase( $query );

    $id = -1;

    if( mysqli_num_rows( $result ) > 0 ) {    
        $id = fs_mysqli_result( $result, 0, "id" );
        }

    if( $id == -1 ) {
        return $startingScore;
        }

    
    global $numLivesInAverage;

    // don't include lives in average that didn't affect our score
    // (like Eve lives, tutorial lives, etc.)
    $query = "SELECT life_id FROM $tableNamePrefix"."offspring ".
        "WHERE player_id = $id AND relation_name = 'You' ".
        "AND old_score != new_score ".
        "order by death_time desc limit $numLivesInAverage;";

    $result = fs_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );
    
    $lifeTotal = 0;
    $numCounted = 0;
    for( $i=0; $i<$numRows; $i++ ) {
        $life_id = fs_mysqli_result( $result, $i, "life_id" );

        $query = "SELECT age FROM $tableNamePrefix"."lives ".
            "WHERE id = $life_id;";

        $resultB = fs_queryDatabase( $query );
        
        $numRowsB = mysqli_num_rows( $result );

        if( $numRowsB > 0 ) {
            $age = fs_mysqli_result( $resultB, $i, "age" );

            $lifeTotal += $age;
            $numCounted ++;
            }
        }

    if( $numCounted < $numLivesInAverage ) {

        // pad average
        for( $i=$numCounted; $i<$numLivesInAverage; $i++ ) {
            $lifeTotal += $startingScore;
            }   
        }

    $ave = $lifeTotal / $numLivesInAverage;

    return $ave;
    }



function fs_reportDeath() {
    fs_checkAndUpdateServerSeqNumber();
    
    
    global $tableNamePrefix;


    $email = fs_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i", "" );

    if( $email == "" ) {
        echo "DENIED";
        die();
        }

    $age = fs_requestFilter( "age", "/[0-9.]+/i", "0" );
    $display_id = fs_requestFilter( "display_id", "/[0-9]+/i", "0" );

    $name = fs_requestFilter( "name", "/[A-Z ]+/i", "" );

    $name = ucwords( strtolower( $name ) );
    
    $name = preg_replace( '/ /', '_', $name );

    
    $player_id = fs_getUserID( $email );
    
    
    $query = "INSERT INTO $tableNamePrefix". "lives SET " .
        "life_player_id = $player_id, ".
        "name = '$name', ".
        "age = $age, ".
        "display_id = $display_id;";
    fs_queryDatabase( $query );

    
    global $fs_mysqlLink;
    $life_id = mysqli_insert_id( $fs_mysqlLink );
    
    $self_rel_name = fs_requestFilter( "self_rel_name", "/[A-Z ]+/i", "You" );



    


    $numAncestors = 0;
    
    $ancestor_list = "";
    if( isset( $_REQUEST[ "ancestor_list" ] ) ) {
        $ancestor_list = $_REQUEST[ "ancestor_list" ];
        }

    $deadPlayerScore = fs_getPlayerScore( $email );
    
    
    if( $ancestor_list != "" ) {
        
        $listParts = explode( ",", $ancestor_list );

        foreach( $listParts as $part ) {

            list( $ancestorEmail, $relName ) = explode( " ", $part, 2 );


            // watch for malformed emails in list
            $ancestorEmail =
                fs_filter( $ancestorEmail,
                           "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i", "" );

            if( $ancestorEmail != "" ) {
                fs_logDeath( $ancestorEmail, $life_id, $relName, $age,
                             $deadPlayerScore );
                }
            $numAncestors ++;
            }
        }


    // log effect of own death
    $noScore = false;
    if( $numAncestors == 0 && $age > 10 ) {
        // this is an Eve or a tutorial Eve player
        // their lifespan doesn't count toward their score
        // all babies (even suicide babies) count, though
        $noScore = true;
        }
    
    fs_logDeath( $email, $life_id, $self_rel_name, $age, $deadPlayerScore,
                 $noScore );


    fs_cleanOldLives( $email );
    
    echo "OK";
    }





function fs_getScore() {
    fs_checkAndUpdateServerSeqNumber();

    global $tableNamePrefix;


    $email = fs_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i", "" );

    if( $email == "" ) {
        echo "DENIED";
        die();
        }

    $query = "SELECT score FROM $tableNamePrefix"."users ".
        "WHERE email = '$email';";

    $result = fs_queryDatabase( $query );

    global $startingScore;
    
    $score = $startingScore;

    if( mysqli_num_rows( $result ) > 0 ) {    
        $score = fs_mysqli_result( $result, 0, "score" );
        }
    
    echo "$score\nOK";
    }



function fs_outputBasicScore( $inEmail ) {
    global $tableNamePrefix;

    // get score with 10 precision, so that we're less likely to
    // include ourselves when doing > comparison to compute rank
    // (due to rounding)
    $query = "SELECT leaderboard_name, ROUND( score, 10  ) as score, ".
        "TIMESTAMPDIFF( SECOND, last_action_time, CURRENT_TIMESTAMP ) ".
        "   as sec_passed ".
        "FROM $tableNamePrefix"."users ".
        "WHERE email = '$inEmail';";

    $result = fs_queryDatabase( $query );

    global $leaderboardHours;
    
    if( mysqli_num_rows( $result ) > 0 ) {    
        $score = fs_mysqli_result( $result, 0, "score" );
        $leaderboard_name = fs_mysqli_result( $result, 0, "leaderboard_name" );
        $sec_passed = fs_mysqli_result( $result, 0, "sec_passed" );

        $leaderboard_name = preg_replace( '/ /', '_', $leaderboard_name );
        
        echo "$leaderboard_name\n$score\n";

        // compute rank

        $rank = 0;

        if( $sec_passed < 3600 * $leaderboardHours ) {
        
            $query =
                "SELECT count(*) FROM $tableNamePrefix"."users ".
                "WHERE last_action_time > ".
                "DATE_SUB( NOW(), INTERVAL $leaderboardHours HOUR ) ".
                "AND score > $score;";

            $result = fs_queryDatabase( $query );

            $rank = 1 + fs_mysqli_result( $result, 0, 0 );
            }
        echo "$rank\n";
        }
    }




function fs_getClientScore() {
    fs_checkAndUpdateClientSeqNumber();

    global $tableNamePrefix;


    $email = fs_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i", "" );

    if( $email == "" ) {
        echo "DENIED";
        die();
        }

    fs_outputBasicScore( $email );

    echo "OK";
    }



function fs_getClientScoreDetails() {
    fs_checkAndUpdateClientSeqNumber();

    global $tableNamePrefix;


    $email = fs_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i", "" );

    if( $email == "" ) {
        echo "DENIED";
        die();
        }

    fs_outputBasicScore( $email );

    // now offspring that contribute to score


    $query = "SELECT id ".
        "FROM $tableNamePrefix"."users ".
        "WHERE email = '$email';";
    $result = fs_queryDatabase( $query );

    $id = fs_mysqli_result( $result, 0, "id" );

    global $maxOffspringToShowPlayer;
    
    $query = "SELECT name, age, display_id, relation_name, ".
        "old_score, new_score, ".
        "TIMESTAMPDIFF( SECOND, death_time, CURRENT_TIMESTAMP ) ".
        "   as died_sec_ago ".
        "FROM $tableNamePrefix"."offspring AS offspring ".
        "INNER JOIN $tableNamePrefix"."lives AS lives ".
        "ON offspring.life_id = lives.id ".
        "WHERE offspring.player_id = $id ORDER BY offspring.death_time DESC ".
        "LIMIT $maxOffspringToShowPlayer";

    
    $result = fs_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );
    
    for( $i=0; $i<$numRows; $i++ ) {
        $name = fs_mysqli_result( $result, $i, "name" );
        $age = fs_mysqli_result( $result, $i, "age" );
        $display_id = fs_mysqli_result( $result, $i, "display_id" );
        $relation_name = fs_mysqli_result( $result, $i, "relation_name" );
        $old_score = fs_mysqli_result( $result, $i, "old_score" );
        $new_score = fs_mysqli_result( $result, $i, "new_score" );
        $died_sec_ago = fs_mysqli_result( $result, $i, "died_sec_ago" );
        // one per line
        // name,relation,display_id,died_sec_ago,age,old_score,new_score

        echo "$name,$relation_name,$display_id,".
            "$died_sec_ago,$age,$old_score,$new_score\n";
        }
    
    
    echo "OK";
    }






$fs_mysqlLink;


// general-purpose functions down here, many copied from seedBlogs

/**
 * Connects to the database according to the database variables.
 */  
function fs_connectToDatabase() {
    global $databaseServer,
        $databaseUsername, $databasePassword, $databaseName,
        $fs_mysqlLink;
    
    
    $fs_mysqlLink =
        mysqli_connect( $databaseServer, $databaseUsername, $databasePassword )
        or fs_operationError( "Could not connect to database server: " .
                              mysqli_error( $fs_mysqlLink ) );
    
    mysqli_select_db( $fs_mysqlLink, $databaseName )
        or fs_operationError( "Could not select $databaseName database: " .
                              mysqli_error( $fs_mysqlLink ) );
    }


 
/**
 * Closes the database connection.
 */
function fs_closeDatabase() {
    global $fs_mysqlLink;
    
    mysqli_close( $fs_mysqlLink );
    }


/**
 * Returns human-readable summary of a timespan.
 * Examples:  10.5 hours
 *            34 minutes
 *            45 seconds
 */
function fs_secondsToTimeSummary( $inSeconds ) {
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
function fs_secondsToAgeSummary( $inSeconds ) {
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
function fs_queryDatabase( $inQueryString ) {
    global $fs_mysqlLink;
    
    if( gettype( $fs_mysqlLink ) != "resource" ) {
        // not a valid mysql link?
        fs_connectToDatabase();
        }
    
    $result = mysqli_query( $fs_mysqlLink, $inQueryString );
    
    if( $result == FALSE ) {

        $errorNumber = mysqli_errno( $fs_mysqlLink );
        
        // server lost or gone?
        if( $errorNumber == 2006 ||
            $errorNumber == 2013 ||
            // access denied?
            $errorNumber == 1044 ||
            $errorNumber == 1045 ||
            // no db selected?
            $errorNumber == 1046 ) {

            // connect again?
            fs_closeDatabase();
            fs_connectToDatabase();

            $result = mysqli_query( $fs_mysqlLink, $inQueryString )
                or fs_operationError(
                    "Database query failed:<BR>$inQueryString<BR><BR>" .
                    mysqli_error( $fs_mysqlLink ) );
            }
        else {
            // some other error (we're still connected, so we can
            // add log messages to database
            fs_fatalError( "Database query failed:<BR>$inQueryString<BR><BR>" .
                           mysqli_error( $fs_mysqlLink ) );
            }
        }

    return $result;
    }



/**
 * Replacement for the old mysql_result function.
 */
function fs_mysqli_result( $result, $number, $field=0 ) {
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
function fs_doesTableExist( $inTableName ) {
    // check if our table exists
    $tableExists = 0;
    
    $query = "SHOW TABLES";
    $result = fs_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );


    for( $i=0; $i<$numRows && ! $tableExists; $i++ ) {

        $tableName = fs_mysqli_result( $result, $i, 0 );
        
        if( $tableName == $inTableName ) {
            $tableExists = 1;
            }
        }
    return $tableExists;
    }



function fs_log( $message ) {
    global $enableLog, $tableNamePrefix, $fs_mysqlLink;

    if( $enableLog ) {
        $slashedMessage = mysqli_real_escape_string( $fs_mysqlLink, $message );
    
        $query = "INSERT INTO $tableNamePrefix"."log VALUES ( " .
            "'$slashedMessage', CURRENT_TIMESTAMP );";
        $result = fs_queryDatabase( $query );
        }
    }



/**
 * Displays the error page and dies.
 *
 * @param $message the error message to display on the error page.
 */
function fs_fatalError( $message ) {
    //global $errorMessage;

    // set the variable that is displayed inside error.php
    //$errorMessage = $message;
    
    //include_once( "error.php" );

    // for now, just print error message
    $logMessage = "Fatal error:  $message";
    
    echo( $logMessage );

    fs_log( $logMessage );
    
    die();
    }



/**
 * Displays the operation error message and dies.
 *
 * @param $message the error message to display.
 */
function fs_operationError( $message ) {
    
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
function fs_addslashes_deep( $inValue ) {
    return
        ( is_array( $inValue )
          ? array_map( 'fs_addslashes_deep', $inValue )
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
function fs_stripslashes_deep( $inValue ) {
    return
        ( is_array( $inValue )
          ? array_map( 'fs_stripslashes_deep', $inValue )
          : stripslashes( $inValue ) );
    }



/**
 * Filters a $_REQUEST variable using a regex match.
 *
 * Returns "" (or specified default value) if there is no match.
 */
function fs_requestFilter( $inRequestVariable, $inRegex, $inDefault = "" ) {
    if( ! isset( $_REQUEST[ $inRequestVariable ] ) ) {
        return $inDefault;
        }

    return fs_filter( $_REQUEST[ $inRequestVariable ], $inRegex, $inDefault );
    }


/**
 * Filters a value  using a regex match.
 *
 * Returns "" (or specified default value) if there is no match.
 */
function fs_filter( $inValue, $inRegex, $inDefault = "" ) {
    
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
function fs_checkPassword( $inFunctionName ) {
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
        $password = fs_hmac_sha1( $passwordHashingPepper,
                                  $_REQUEST[ "passwordHMAC" ] );
        
        
        // generate a new hash cookie from this password
        $newSalt = time();
        $newHash = md5( $newSalt . $password );
        
        $password_hash = $newSalt . "_" . $newHash;
        }
    else if( isset( $_COOKIE[ $cookieName ] ) ) {
        fs_checkReferrer();
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

            fs_log( "Failed $inFunctionName access with password:  ".
                    "$password" );
            }
        else {
            echo "Session expired.";
                
            fs_log( "Failed $inFunctionName access with bad cookie:  ".
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
            
            
            $nonce = fs_hmac_sha1( $passwordHashingPepper, uniqid() );
            
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
 



function fs_clearPasswordCookie() {
    global $tableNamePrefix;

    $cookieName = $tableNamePrefix . "cookie_password_hash";

    // expire 24 hours ago (to avoid timezone issues)
    $expireTime = time() - 60 * 60 * 24;

    setcookie( $cookieName, "", $expireTime, "/" );
    }
 
 







function fs_hmac_sha1( $inKey, $inData ) {
    return hash_hmac( "sha1", 
                      $inData, $inKey );
    } 

 
function fs_hmac_sha1_raw( $inKey, $inData ) {
    return hash_hmac( "sha1", 
                      $inData, $inKey, true );
    } 


 
 
 
 
// decodes a ASCII hex string into an array of 0s and 1s 
function fs_hexDecodeToBitString( $inHexString ) {
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
