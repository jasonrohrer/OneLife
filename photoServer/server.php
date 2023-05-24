<?php



global $ps_version;
$ps_version = "1";



// edit settings.php to change server' settings
include( "settings.php" );




// no end-user settings below this point








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
<HEAD><TITLE>Photo Server Web-based setup</TITLE></HEAD>
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
    $_GET     = array_map( 'ps_stripslashes_deep', $_GET );
    $_POST    = array_map( 'ps_stripslashes_deep', $_POST );
    $_REQUEST = array_map( 'ps_stripslashes_deep', $_REQUEST );
    $_COOKIE  = array_map( 'ps_stripslashes_deep', $_COOKIE );
    }
    


// Check that the referrer header is this page, or kill the connection.
// Used to block XSRF attacks on state-changing functions.
// (To prevent it from being dangerous to surf other sites while you are
// logged in as admin.)
// Thanks Chris Cowan.
function ps_checkReferrer() {
    global $fullServerURL;
    
    if( !isset($_SERVER['HTTP_REFERER']) ||
        strpos($_SERVER['HTTP_REFERER'], $fullServerURL) !== 0 ) {
        
        die( "Bad referrer header" );
        }
    }




// all calls need to connect to DB, so do it once here
ps_connectToDatabase();

// close connection down below (before function declarations)


// testing:
//sleep( 5 );


// general processing whenver server.php is accessed directly




// grab POST/GET variables
$action = ps_requestFilter( "action", "/[A-Z_]+/i" );

$debug = ps_requestFilter( "debug", "/[01]/" );

$remoteIP = "";
if( isset( $_SERVER[ "REMOTE_ADDR" ] ) ) {
    $remoteIP = $_SERVER[ "REMOTE_ADDR" ];
    }


if( $action != "photo_link_image" ) {
    // no caching, EXCEPT for generated images
    
    //header('Expires: Mon, 26 Jul 1997 05:00:00 GMT');
    header('Cache-Control: no-store, no-cache, must-revalidate');
    header('Cache-Control: post-check=0, pre-check=0', FALSE);
    header('Pragma: no-cache'); 

    // note that the photo_link_status should never change for a a dead
    // player, and only the lineage server (which talks about dead players only)
    // will use these link images
    // (Players can't appear in a photo after they're dead)
    // So caching these link images is okay.
    }
else {
    header("Cache-Control: private, max-age=10800, pre-check=10800");
    header("Pragma: private");
    header("Expires: " . date(DATE_RFC822,strtotime(" 20 day")));

    if( isset($_SERVER['HTTP_IF_MODIFIED_SINCE'] ) ) { 
        header('Last-Modified: '.
               date(DATE_RFC822,strtotime("26 April 2019")), true, 304);
        exit;
        }
    else {
        header("Last-Modified: " .
               date(DATE_RFC822,strtotime("26 April 2019")));
        }
    }




if( $action == "version" ) {
    global $ps_version;
    echo "$ps_version";
    }
else if( $action == "get_sequence_number" ) {
    ps_getSequenceNumber();
    }
else if( $action == "submit_photo" ) {
    ps_submitPhoto();
    }
else if( $action == "photo_link_image" ) {
    ps_photoLinkImage();
    }
else if( $action == "photo_appearances" ) {
    ps_photoAppearances();
    }
else if( $action == "front_page" ) {
    ps_frontPage();
    }
else if( $action == "show_log" ) {
    ps_showLog();
    }
else if( $action == "clear_log" ) {
    ps_clearLog();
    }
else if( $action == "show_data" ) {
    ps_showData();
    }
else if( $action == "show_detail" ) {
    ps_showDetail();
    }
else if( $action == "delete_photo" ) {
    ps_deletePhoto();
    }
else if( $action == "logout" ) {
    ps_logout();
    }
else if( $action == "ps_setup" ) {
    global $setup_header, $setup_footer;
    echo $setup_header; 

    echo "<H2>Photo Server Web-based Setup</H2>";

    echo "Creating tables:<BR>";

    echo "<CENTER><TABLE BORDER=0 CELLSPACING=0 CELLPADDING=1>
          <TR><TD BGCOLOR=#000000>
          <TABLE BORDER=0 CELLSPACING=0 CELLPADDING=5>
          <TR><TD BGCOLOR=#FFFFFF>";

    ps_setupDatabase();

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
    $exists = ps_doesTableExist( $tableNamePrefix . "photos" ) &&
        ps_doesTableExist( $tableNamePrefix . "users" ) &&
        ps_doesTableExist( $tableNamePrefix . "photo_appearances" ) &&
        ps_doesTableExist( $tableNamePrefix . "log" );
    
        
    if( $exists  ) {
        echo "Photo Server database setup and ready";
        }
    else {
        // start the setup procedure

        global $setup_header, $setup_footer;
        echo $setup_header; 

        echo "<H2>Photo Server Web-based Setup</H2>";
    
        echo "Photo Server will walk you through a " .
            "brief setup process.<BR><BR>";
        
        echo "Step 1: ".
            "<A HREF=\"server.php?action=ps_setup\">".
            "create the database tables</A>";

        echo $setup_footer;
        }
    }



// done processing
// only function declarations below

ps_closeDatabase();







/**
 * Creates the database tables needed by seedBlogs.
 */
function ps_setupDatabase() {
    global $tableNamePrefix;

    $tableName = $tableNamePrefix . "log";
    if( ! ps_doesTableExist( $tableName ) ) {

        // this table contains general info about the server
        // use INNODB engine so table can be locked
        $query =
            "CREATE TABLE $tableName(" .
            "entry TEXT NOT NULL, ".
            "entry_time DATETIME NOT NULL, ".
            "index( entry_time ) );";

        $result = ps_queryDatabase( $query );

        echo "<B>$tableName</B> table created<BR>";
        }
    else {
        echo "<B>$tableName</B> table already exists<BR>";
        }

    
    
    $tableName = $tableNamePrefix . "users";
    if( ! ps_doesTableExist( $tableName ) ) {

        $query =
            "CREATE TABLE $tableName(" .
            "id INT UNSIGNED NOT NULL PRIMARY KEY AUTO_INCREMENT," .
            "email VARCHAR(254) NOT NULL," .
            "UNIQUE KEY( email )," .
            "sequence_number INT NOT NULL," .
            "photos_submitted INT NOT NULL," .
            "photos_rejected INT NOT NULL );";

        $result = ps_queryDatabase( $query );

        echo "<B>$tableName</B> table created<BR>";
        }
    else {
        echo "<B>$tableName</B> table already exists<BR>";
        }



    $tableName = $tableNamePrefix . "photos";
    if( ! ps_doesTableExist( $tableName ) ) {

        $query =
            "CREATE TABLE $tableName(" .
            "id INT UNSIGNED NOT NULL PRIMARY KEY AUTO_INCREMENT," .
            "user_id INT UNSIGNED NOT NULL,".
            "index( user_id ), ".
            "url VARCHAR(254) NOT NULL,".
            "author_name varchar(255) NOT NULL,".
            "subject_names text NOT NULL,".
            "submission_time DATETIME NOT NULL );";

        $result = ps_queryDatabase( $query );

        echo "<B>$tableName</B> table created<BR>";
        }
    else {
        echo "<B>$tableName</B> table already exists<BR>";
        }


    $tableName = $tableNamePrefix . "photo_appearances";
    if( ! ps_doesTableExist( $tableName ) ) {

        $query =
            "CREATE TABLE $tableName(" .
            "id INT UNSIGNED NOT NULL PRIMARY KEY AUTO_INCREMENT," .
            "server_name VARCHAR(254) NOT NULL," .
            "player_id INT UNSIGNED NOT NULL," .
            "index( server_name, player_id )," .
            "photo_id INT UNSIGNED NOT NULL );";
        
        $result = ps_queryDatabase( $query );

        echo "<B>$tableName</B> table created<BR>";
        }
    else {
        echo "<B>$tableName</B> table already exists<BR>";
        }
    }



function ps_showLog() {
    ps_checkPassword( "show_log" );

     echo "[<a href=\"server.php?action=show_data" .
         "\">Main</a>]<br><br><br>";
    
    global $tableNamePrefix;

    $query = "SELECT * FROM $tableNamePrefix"."log ".
        "ORDER BY entry_time DESC;";
    $result = ps_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );



    echo "<a href=\"server.php?action=clear_log\">".
        "Clear log</a>";
        
    echo "<hr>";
        
    echo "$numRows log entries:<br><br><br>\n";
        

    for( $i=0; $i<$numRows; $i++ ) {
        $time = ps_mysqli_result( $result, $i, "entry_time" );
        $entry = htmlspecialchars( ps_mysqli_result( $result, $i, "entry" ) );

        echo "<b>$time</b>:<br>$entry<hr>\n";
        }
    }



function ps_clearLog() {
    ps_checkPassword( "clear_log" );

     echo "[<a href=\"server.php?action=show_data" .
         "\">Main</a>]<br><br><br>";
    
    global $tableNamePrefix;

    $query = "DELETE FROM $tableNamePrefix"."log;";
    $result = ps_queryDatabase( $query );
    
    if( $result ) {
        echo "Log cleared.";
        }
    else {
        echo "DELETE operation failed?";
        }
    }




















function ps_logout() {
    ps_checkReferrer();
    
    ps_clearPasswordCookie();

    echo "Logged out";
    }




function ps_showData( $checkPassword = true ) {
    // these are global so they work in embeded function call below
    global $skip, $search, $order_by;

    if( $checkPassword ) {
        ps_checkPassword( "show_data" );
        }
    
    global $tableNamePrefix, $remoteIP;
    

    echo "<table width='100%' border=0><tr>".
        "<td>[<a href=\"server.php?action=show_data" .
            "\">Main</a>]</td>".
        "<td align=right>[<a href=\"server.php?action=logout" .
            "\">Logout</a>]</td>".
        "</tr></table><br><br><br>";




    $skip = ps_requestFilter( "skip", "/[0-9]+/", 0 );
    
    global $usersPerPage;
    
    $search = ps_requestFilter( "search", "/[A-Z0-9_@. \-]+/i" );

    $order_by = ps_requestFilter( "order_by", "/[A-Z_]+/i",
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

    $result = ps_queryDatabase( $query );
    $totalRecords = ps_mysqli_result( $result, 0, 0 );


    $orderDir = "DESC";

    if( $order_by == "email" ) {
        $orderDir = "ASC";
        }
    
             
    $query = "SELECT * ".
        "FROM $tableNamePrefix"."users $keywordClause".
        "ORDER BY $order_by $orderDir ".
        "LIMIT $skip, $usersPerPage;";
    $result = ps_queryDatabase( $query );
    
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
    echo "<td>".orderLink( "photos_submitted", "Photos Submitted" )."</td>\n";
    echo "<td>".orderLink( "photos_rejected", "Photos Rejected" )."</td>\n";
    echo "</tr>\n";


    for( $i=0; $i<$numRows; $i++ ) {
        $id = ps_mysqli_result( $result, $i, "id" );
        $email = ps_mysqli_result( $result, $i, "email" );
        $photos_submitted = ps_mysqli_result( $result, $i, "photos_submitted" );
        $photos_rejected = ps_mysqli_result( $result, $i, "photos_rejected" );
        
        $encodedEmail = urlencode( $email );

        
        echo "<tr>\n";

        echo "<td>$id</td>\n";
        echo "<td>".
            "<a href=\"server.php?action=show_detail&email=$encodedEmail\">".
            "$email</a></td>\n";
        echo "<td>$photos_submitted</td>\n";
        echo "<td>$photos_rejected</td>\n";
        echo "</tr>\n";
        }
    echo "</table>";


    echo "<hr>";
    
    echo "<a href=\"server.php?action=show_log\">".
        "Show log</a>";
    echo "<hr>";
    echo "Generated for $remoteIP\n";

    }



function ps_showDetail( $checkPassword = true ) {
    if( $checkPassword ) {
        ps_checkPassword( "show_detail" );
        }
    echo "<body bgcolor=#AAAAAA>";
    
    echo "[<a href=\"server.php?action=show_data" .
         "\">Main</a>]<br><br><br>";
    
    global $tableNamePrefix;
    

    $email = ps_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i" );
            
    $query = "SELECT id, photos_submitted, photos_rejected ".
        "FROM $tableNamePrefix"."users ".
        "WHERE email = '$email';";
    $result = ps_queryDatabase( $query );

    $id = ps_mysqli_result( $result, 0, "id" );
    $photos_submitted = ps_mysqli_result( $result, 0, "photos_submitted" );
    $photos_rejected = ps_mysqli_result( $result, 0, "photos_rejected" );
    
    

    echo "<center><table border=0><tr><td>";
    
    echo "<b>ID:</b> $id<br><br>";
    echo "<b>Email:</b> $email<br><br>";
    echo "<b>Photos Submitted:</b> $photos_submitted<br><br>";
    echo "<b>Photos Rejected:</b> $photos_rejected<br><br>";
    echo "<br><br>";


    ps_displayPhotoList( "WHERE user_id = '$id'", "", true, $email );

    echo "</td></tr></table></center>";
    echo "</body>";
    }



function ps_deletePhoto() {
    if( $checkPassword ) {
        ps_checkPassword( "delete_photo" );
        }
    
    global $tableNamePrefix;
    

    $email = ps_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i" );
    $id = ps_requestFilter( "id", "/[0-9]+/i", -1 );

    if( ! $id != -1 ) {
        $query = "DELETE FROM $tableNamePrefix"."photos ".
            "WHERE id = $id;";
        ps_queryDatabase( $query );

        $query = "DELETE FROM $tableNamePrefix"."photo_appearances ".
            "WHERE photo_id = $id;";
        ps_queryDatabase( $query );

        $query = "UPDATE $tableNamePrefix"."users ".
            "SET photos_submitted = photos_submitted - 1, ".
            "photos_rejected = photos_rejected + 1 ".
            "WHERE email = '$email';";
        ps_queryDatabase( $query );
        }
    
    ps_showDetail( false );
    }



function ps_displayPhotoList( $inWhereClause, $inLimitClause,
                              $inShowDeleteLinks = false,
                              $inShowDeleteEmail = "" ) {
    global $tableNamePrefix;
    
    $query = "SELECT id, url, author_name, subject_names, submission_time ".
        "FROM $tableNamePrefix"."photos ".
        "$inWhereClause ".
        "ORDER BY id DESC $inLimitClause;";
    $result = ps_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );
    

    for( $i=0; $i<$numRows; $i++ ) {
        $id = ps_mysqli_result( $result, $i, "id" );
        $url = ps_mysqli_result( $result, $i, "url" );
        $author_name = ps_mysqli_result( $result, $i, "author_name" );
        $subject_names = ps_mysqli_result( $result, $i, "subject_names" );
        $submission_time = ps_mysqli_result( $result, $i, "submission_time" );


        if( $inShowDeleteLinks ) {
            echo "<table border=0><tr><td>";
            }
            
        ps_displayPhoto( $url, $author_name, $subject_names,
                         $submission_time );

        if( $inShowDeleteLinks ) {
            echo "</td><td>[<a href=\"server.php?action=delete_photo".
                "&email=$inShowDeleteEmail&id=$id\">delete</a>]".
                "</td></tr></table>";
            }
        }
    }



function ps_displayPhoto( $url, $author_name, $subject_names,
                          $submission_time ) {    
    
    echo "<table width=400 border=0><tr><td colspan=2>";
        
    echo "<img width=400 height=400 src='$url'></td></tr>";

        
    echo "<tr><td>by $author_name</td>";

    $agoSec = strtotime( "now" ) - strtotime( $submission_time );
        
    $ago = ps_secondsToAgeSummary( $agoSec );

    echo "<td align=right>$ago ago</td></tr><br>";

    if( $subject_names != "" ) {
        echo "<tr><td colspan=2>featuring $subject_names</td></tr>";
        }
    echo "</table>";
        
    echo "<br><br><br>\n";
    }



function ps_displayPhotoID( $id ) {
    global $tableNamePrefix;
    
    $query = "SELECT url, author_name, subject_names, submission_time ".
        "FROM $tableNamePrefix"."photos ".
        "WHERE id = $id;";
    $result = ps_queryDatabase( $query );

    $url = ps_mysqli_result( $result, 0, "url" );
    $author_name = ps_mysqli_result( $result, 0, "author_name" );
    $subject_names = ps_mysqli_result( $result, 0, "subject_names" );
    $submission_time = ps_mysqli_result( $result, 0, "submission_time" );

    ps_displayPhoto( $url, $author_name, $subject_names, $submission_time );
    }







function ps_getSequenceNumber() {
    global $tableNamePrefix;
    

    $email = ps_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i", "" );

    if( $email == "" ) {
        ps_log( "getSequenceNumber denied for bad email" );

        echo "DENIED";
        return;
        }
    
    
    $seq = ps_getSequenceNumberForEmail( $email );

    echo "$seq\n"."OK";
    }



// assumes already-filtered, valid email
// returns 0 if not found
function ps_getSequenceNumberForEmail( $inEmail ) {
    global $tableNamePrefix;
    
    $query = "SELECT sequence_number FROM $tableNamePrefix"."users ".
        "WHERE email = '$inEmail';";
    $result = ps_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );

    if( $numRows < 1 ) {
        return 0;
        }
    else {
        return ps_mysqli_result( $result, 0, "sequence_number" );
        }
    }




function ps_photoLinkImage() {
    global $tableNamePrefix, $photosLinkImage, $noPhotosLinkImage;
    
    $server_name = ps_requestFilter( "server_name", "/[A-Z0-9._\-]+/i", "" );
    $player_id = ps_requestFilter( "player_id", "/[0-9]+/i", "0" );

    // count results
    $query = "SELECT COUNT(*) FROM $tableNamePrefix".
        "photo_appearances ".
        "WHERE server_name = '$server_name' AND player_id = $player_id;";

    $result = ps_queryDatabase( $query );
    $totalRecords = ps_mysqli_result( $result, 0, 0 );

    header("Content-type: image/png");
    
    if( $totalRecords == 0 ) {
        readfile( $noPhotosLinkImage );
        }
    else {
        readfile( $photosLinkImage );
        }
    }



function ps_photoAppearances() {
    global $tableNamePrefix;
    
    $server_name = ps_requestFilter( "server_name", "/[A-Z0-9._\-]+/i", "" );
    $player_id = ps_requestFilter( "player_id", "/[0-9]+/i", "0" );

    // count results
    $query = "SELECT photo_id FROM $tableNamePrefix".
        "photo_appearances ".
        "WHERE server_name = '$server_name' AND player_id = $player_id;";

    $result = ps_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );

    global $header, $footer;

    eval( $header );

    echo "<center>";
    
    if( $numRows == 0 ) {
        echo "No photos found";
        }
    else {
        for( $i=0; $i<$numRows; $i++ ) {
            $id = ps_mysqli_result( $result, $i, "photo_id" );
            ps_displayPhotoID( $id );
            }
        }

    echo "</center>";
    
    eval( $footer );
    }



function ps_frontPage() {

    $page = ps_requestFilter( "page", "/[0-9]+/i", "0" );

    global $tableNamePrefix;
    
    $query = "SELECT COUNT(*) ".
        "FROM $tableNamePrefix"."photos;";
    
    $result = ps_queryDatabase( $query );
    $totalPhotos = ps_mysqli_result( $result, 0, 0 );
        
        
    global $header, $footer, $fullServerURL;

    eval( $header );

    $numPerPage = 10;
    $skipAmount = $numPerPage * $page;

    
    echo "<center>";

    echo "<br><font size=5>Recent Photographs:</font><br><br>";

    echo "<table border=0 width=400>";
    echo "<tr><td width=50%>";

    if( $page > 0 ) {
        // prev button
        $prevPage = $page - 1;
        echo "[<a href=$fullServerURL?action=front_page".
            "&page=$prevPage>Previous</a>]";
        }
    echo "</td>";
    echo "<td width=50% align=right>";
    
    if( $totalPhotos > $skipAmount + $numPerPage ) {
        // next button
        $nextPage = $page + 1;
        echo "[<a href=$fullServerURL?action=front_page".
            "&page=$nextPage>Next</a>]";
        }
    echo "</td></tr></table>";

    
    ps_displayPhotoList( "", "LIMIT $skipAmount, $numPerPage" );



    echo "<table border=0 width=400>";
    echo "<tr><td width=50%>";

    if( $page > 0 ) {
        // prev button
        $prevPage = $page - 1;
        echo "[<a href=$fullServerURL?action=front_page".
            "&page=$prevPage>Previous</a>]";
        }
    echo "</td>";
    echo "<td width=50% align=right>";
    
    if( $totalPhotos > $skipAmount + $numPerPage ) {
        // next button
        $nextPage = $page + 1;
        echo "[<a href=$fullServerURL?action=front_page".
            "&page=$nextPage>Next</a>]";
        }
    echo "</td></tr></table>";
    
    echo "</center><br><br><br>";

    
    eval( $footer );
    }



function ps_submitPhoto() {
    /*
    &email=[email address]
    &sequence_number=[int]
    &hash_value=[hash value]
    &server_sig=[hash value]
    &server_name=[string]
    &photo_author_id=[int]
    &photo_subjects_ids=[string]
    &photo_author_name=[string]
    &photo_subjects_names=[string]
    &jpg_base64=[jpg file as base 64]
    */

    global $tableNamePrefix;
    
    $email = ps_requestFilter( "email", "/[A-Z0-9._%+\-]+@[A-Z0-9.\-]+/i", "" );
    $sequence_number = ps_requestFilter( "sequence_number", "/[0-9]+/i", "0" );

    if( $email == "" ) {
        echo "DENIED";
        return;
        }

    if( ps_getSequenceNumberForEmail( $email ) > $sequence_number ) {
        echo "DENIED";
        return;
        }

    
    $id = ps_getUserID( $email );

    
    
    $hash_value = ps_requestFilter( "hash_value", "/[A-F0-9]+/i", "" );

    $hash_value = strtoupper( $hash_value );

    
    $server_sig = ps_requestFilter( "server_sig", "/[A-F0-9]+/i", "" );

    $server_sig = strtoupper( $server_sig );

    $server_name = ps_requestFilter( "server_name", "/[A-Z0-9._\-]+/i", "" );


    global $sharedGameServerSecret;

    $computedServerSig =
        strtoupper( ps_hmac_sha1( $sharedGameServerSecret, $sequence_number ) );

    if( $server_sig != $computedServerSig ) {
        // don't log this, because it might be common from clients
        // that are playing on unofficial servers
        
        ps_log( "submitPhoto denied for bad server sig, ".
                "$email on $server_name" );

        // don't count these as rejected either
        // want rejected to be a sign that people are trying to hack
        /*
        if( $id != -1 ) {
            $query = "UPDATE $tableNamePrefix"."users ".
                "SET photos_rejected = photos_rejected + 1;";
            ps_queryDatabase( $query );
            }
        */
        
        echo "DENIED";
        return;
        }
    


    $encodedEmail = urlencode( $email );

    global $ticketServerURL;

    $request = "$ticketServerURL".
        "?action=check_ticket_hash".
        "&email=$encodedEmail" .
        "&hash_value=$hash_value" .
        "&string_to_hash=$sequence_number";
    
    $result = file_get_contents( $request );

    if( $result != "VALID" ) {
        ps_log( "submitPhoto denied for failed hash check, $email" );

        if( $id != -1 ) {
            $query = "UPDATE $tableNamePrefix"."users ".
                "SET photos_rejected = photos_rejected + 1 ".
                "WHERE email = '$email';";
            ps_queryDatabase( $query );
            }
        
        echo "DENIED";
        return;
        }

    // base64
    // [0-9], [A-Z], [a-z], and [+,/,=].    

    $jpg_base64 = ps_requestFilter( "jpg_base64", "/[A-Z0-9a-z+\/=]+/", "" );

    $jpg_binary = base64_decode( $jpg_base64 );

    // search for 0xFFEE002A
    // 002A is length 42 (hash length plus 2 length bytes) 
    // followed by 40-char hash in ascii

    $sigComment = hex2bin( "FFEE002A" ) . $hash_value;

    $pos = strpos( $jpg_binary, $sigComment );

    if( $pos === FALSE ) {
        ps_log( "submitPhoto denied for missing hash comment, $email" );

        if( $id != -1 ) {
            $query = "UPDATE $tableNamePrefix"."users ".
                "SET photos_rejected = photos_rejected + 1 ".
                "WHERE email = '$email';";
            ps_queryDatabase( $query );
            }
        else {
            // got this far
            // they are a real user
            // but the photo they submitted was bogus
            // create a record for them to count it
            $query = "INSERT INTO $tableNamePrefix". "users SET " .
                "email = '$email', sequence_number = 1, ".
                "photos_submitted = 0, photos_rejected = 1;";
            ps_queryDatabase( $query );
            $id = ps_getUserID( $email );
            }
        
        echo "DENIED";
        return;
        }

    // jpg submission valid

    // save it into the folder
    global $submittedPhotoLocation, $submittedPhotoURL;

    $photoFileName =
        ps_hmac_sha1( $email, $sequence_number . uniqid() ) . ".jpg";

    $filePath = $submittedPhotoLocation . $photoFileName;

    
    file_put_contents( $filePath, $jpg_binary );

    $photoURL = $submittedPhotoURL . $photoFileName;
    
    $photo_author_id = ps_requestFilter( "photo_author_id", "/[0-9]+/i", "0" );
    $photo_subject_ids =
        ps_requestFilter( "photo_subjects_ids", "/[0-9,]+/i", "0" );
    $photo_author_name =
        ps_requestFilter( "photo_author_name", "/[A-Z ]+/i", "" );
    $photo_subjects_names =
        ps_requestFilter( "photo_subjects_names", "/[A-Z ,]+/i", "" );

    $subjectIDs = explode( ",", $photo_subject_ids );
    $subjectNames = explode( ",", $photo_subjects_names );


    // 1.  insert user record if needed (or update sequence number if not)
    if( $id != -1 ) {
        $query = "UPDATE $tableNamePrefix"."users ".
            "SET sequence_number = sequence_number + 1, ".
            "photos_submitted = photos_submitted + 1 ".
            "WHERE email = '$email';";
        ps_queryDatabase( $query );
        }
    else {
        $query = "INSERT INTO $tableNamePrefix". "users SET " .
            "email = '$email', sequence_number = 1, ".
            "photos_submitted = 1, photos_rejected = 0;";
        ps_queryDatabase( $query );
        $id = ps_getUserID( $email );
        }
    // 2.  insert photo record with names, user ID, and URL
    $photo_author_name = ps_formatName( $photo_author_name );

    $subjectList = "";
    if( $photo_subjects_names != "" && count( $subjectNames ) > 0 ) {
        $subjectList = ps_formatName( $subjectNames[0] );

        if( count( $subjectNames ) == 2 ) {
            $subjectList = $subjectList . " and " .
                ps_formatName( $subjectNames[1] );
            }
        else if( count( $subjectNames ) > 2 ) {
                for( $i=1; $i < count( $subjectNames ) - 1; $i ++ ) {
                    $subjectList = $subjectList . ", " .
                        ps_formatName( $subjectNames[$i] );
                    }
                $subjectList =
                    $subjectList . ", and " .
                    ps_formatName(
                        $subjectNames[ count( $subjectNames ) - 1 ] );
            }
        }
    

    $query = "INSERT INTO $tableNamePrefix"."photos ".
        "SET user_id = $id, url = '$photoURL', ".
        "submission_time = CURRENT_TIMESTAMP, ".
        "author_name = '$photo_author_name', subject_names = '$subjectList';";
    ps_queryDatabase( $query );

    global $ps_mysqlLink;
    
    $photoID = mysqli_insert_id( $ps_mysqlLink );

    
    // 3.  insert photo_appearances records for each subject and author
    //     (probably write a function for this and call it in a loop)
    ps_addAppearance( $photoID, $server_name, $photo_author_id );

    if( $photo_subject_ids != "" ) {
        foreach( $subjectIDs as $subjectID ) {
            ps_addAppearance( $photoID, $server_name, $subjectID );
            }
        }

    echo "OK";
    }




function ps_addAppearance( $photoID, $server_name, $player_id ) {
    global $tableNamePrefix;
    $query = "INSERT INTO $tableNamePrefix"."photo_appearances ".
        "SET server_name = '$server_name', player_id = $player_id, ".
        "photo_id = $photoID;";
    ps_queryDatabase( $query );
    }



// -1 if doesn't exist
function ps_getUserID( $email ) {
    global $tableNamePrefix;
    
    $query = "SELECT id FROM $tableNamePrefix"."users ".
        "WHERE email = '$email';";
    
    $result = ps_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );

    $id = -1;
    
    if( $numRows > 0 ) {
        $id = ps_mysqli_result( $result, 0, "id" );
        }
    return $id;
    }



function ps_formatName( $inName ) {
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
            if( ps_romanToInt( $nameParts[1] ) > 0 ) {
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





$ps_mysqlLink;


// general-purpose functions down here, many copied from seedBlogs

/**
 * Connects to the database according to the database variables.
 */  
function ps_connectToDatabase() {
    global $databaseServer,
        $databaseUsername, $databasePassword, $databaseName,
        $ps_mysqlLink;
    
    
    $ps_mysqlLink =
        mysqli_connect( $databaseServer, $databaseUsername, $databasePassword )
        or ps_operationError( "Could not connect to database server: " .
                              mysqli_error( $ps_mysqlLink ) );
    
    mysqli_select_db( $ps_mysqlLink, $databaseName )
        or ps_operationError( "Could not select $databaseName database: " .
                              mysqli_error( $ps_mysqlLink ) );
    }


 
/**
 * Closes the database connection.
 */
function ps_closeDatabase() {
    global $ps_mysqlLink;
    
    mysqli_close( $ps_mysqlLink );
    }


/**
 * Returns human-readable summary of a timespan.
 * Examples:  10.5 hours
 *            34 minutes
 *            45 seconds
 */
function ps_secondsToTimeSummary( $inSeconds ) {
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
function ps_secondsToAgeSummary( $inSeconds ) {
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
function ps_queryDatabase( $inQueryString ) {
    global $ps_mysqlLink;
    
    if( gettype( $ps_mysqlLink ) != "resource" ) {
        // not a valid mysql link?
        ps_connectToDatabase();
        }
    
    $result = mysqli_query( $ps_mysqlLink, $inQueryString );
    
    if( $result == FALSE ) {

        $errorNumber = mysqli_errno( $ps_mysqlLink );
        
        // server lost or gone?
        if( $errorNumber == 2006 ||
            $errorNumber == 2013 ||
            // access denied?
            $errorNumber == 1044 ||
            $errorNumber == 1045 ||
            // no db selected?
            $errorNumber == 1046 ) {

            // connect again?
            ps_closeDatabase();
            ps_connectToDatabase();

            $result = mysqli_query( $ps_mysqlLink, $inQueryString )
                or ps_operationError(
                    "Database query failed:<BR>$inQueryString<BR><BR>" .
                    mysqli_error( $ps_mysqlLink ) );
            }
        else {
            // some other error (we're still connected, so we can
            // add log messages to database
            ps_fatalError( "Database query failed:<BR>$inQueryString<BR><BR>" .
                           mysqli_error( $ps_mysqlLink ) );
            }
        }

    return $result;
    }



/**
 * Replacement for the old mysql_result function.
 */
function ps_mysqli_result( $result, $number, $field=0 ) {
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
function ps_doesTableExist( $inTableName ) {
    // check if our table exists
    $tableExists = 0;
    
    $query = "SHOW TABLES";
    $result = ps_queryDatabase( $query );

    $numRows = mysqli_num_rows( $result );


    for( $i=0; $i<$numRows && ! $tableExists; $i++ ) {

        $tableName = ps_mysqli_result( $result, $i, 0 );
        
        if( $tableName == $inTableName ) {
            $tableExists = 1;
            }
        }
    return $tableExists;
    }



function ps_log( $message ) {
    global $enableLog, $tableNamePrefix, $ps_mysqlLink;

    if( $enableLog ) {
        $slashedMessage = mysqli_real_escape_string( $ps_mysqlLink, $message );
    
        $query = "INSERT INTO $tableNamePrefix"."log VALUES ( " .
            "'$slashedMessage', CURRENT_TIMESTAMP );";
        $result = ps_queryDatabase( $query );
        }
    }



/**
 * Displays the error page and dies.
 *
 * @param $message the error message to display on the error page.
 */
function ps_fatalError( $message ) {
    //global $errorMessage;

    // set the variable that is displayed inside error.php
    //$errorMessage = $message;
    
    //include_once( "error.php" );

    // for now, just print error message
    $logMessage = "Fatal error:  $message";
    
    echo( $logMessage );

    ps_log( $logMessage );
    
    die();
    }



/**
 * Displays the operation error message and dies.
 *
 * @param $message the error message to display.
 */
function ps_operationError( $message ) {
    
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
function ps_addslashes_deep( $inValue ) {
    return
        ( is_array( $inValue )
          ? array_map( 'ps_addslashes_deep', $inValue )
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
function ps_stripslashes_deep( $inValue ) {
    return
        ( is_array( $inValue )
          ? array_map( 'ps_stripslashes_deep', $inValue )
          : stripslashes( $inValue ) );
    }



/**
 * Filters a $_REQUEST variable using a regex match.
 *
 * Returns "" (or specified default value) if there is no match.
 */
function ps_requestFilter( $inRequestVariable, $inRegex, $inDefault = "" ) {
    if( ! isset( $_REQUEST[ $inRequestVariable ] ) ) {
        return $inDefault;
        }

    return ps_filter( $_REQUEST[ $inRequestVariable ], $inRegex, $inDefault );
    }


/**
 * Filters a value  using a regex match.
 *
 * Returns "" (or specified default value) if there is no match.
 */
function ps_filter( $inValue, $inRegex, $inDefault = "" ) {
    
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
function ps_checkPassword( $inFunctionName ) {
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
        $password = ps_hmac_sha1( $passwordHashingPepper,
                                  $_REQUEST[ "passwordHMAC" ] );
        
        
        // generate a new hash cookie from this password
        $newSalt = time();
        $newHash = md5( $newSalt . $password );
        
        $password_hash = $newSalt . "_" . $newHash;
        }
    else if( isset( $_COOKIE[ $cookieName ] ) ) {
        ps_checkReferrer();
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

            ps_log( "Failed $inFunctionName access with password:  ".
                    "$password" );
            }
        else {
            echo "Session expired.";
                
            ps_log( "Failed $inFunctionName access with bad cookie:  ".
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
            
            
            $nonce = ps_hmac_sha1( $passwordHashingPepper, uniqid() );
            
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
 



function ps_clearPasswordCookie() {
    global $tableNamePrefix;

    $cookieName = $tableNamePrefix . "cookie_password_hash";

    // expire 24 hours ago (to avoid timezone issues)
    $expireTime = time() - 60 * 60 * 24;

    setcookie( $cookieName, "", $expireTime, "/" );
    }
 
 







function ps_hmac_sha1( $inKey, $inData ) {
    return hash_hmac( "sha1", 
                      $inData, $inKey );
    } 

 
function ps_hmac_sha1_raw( $inKey, $inData ) {
    return hash_hmac( "sha1", 
                      $inData, $inKey, true );
    } 


 
 
 
 
// decodes a ASCII hex string into an array of 0s and 1s 
function ps_hexDecodeToBitString( $inHexString ) {
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
