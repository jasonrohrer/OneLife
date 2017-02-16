<?php

include( "forums/config.php" );
include( "forums/include/functions.php" );

// number id of the news forum to pull from
$forumID = 4;





$news_mysqlLink =
    mysql_connect( $db_host, $db_username, $db_password )
    or news_operationError( "Could not connect to database server: " .
                            mysql_error() );

mysql_select_db( $db_name )
or news_operationError( "Could not select $databaseName database: " .
                        mysql_error() );




$pageNumber = news_requestFilter( "page", "/[0-9]+/", 1 );

global $numNewsPerPage;

if( $numNewsPerPage < 1 ) {
    
    $numNewsPerPage = news_requestFilter( "numPerPage", "/[0-9]+/", -1 );
    }

if( $numNewsPerPage < 1 ) {
    $numNewsPerPage = 5;
    }


$numToSkip = ( $pageNumber - 1 ) * $numNewsPerPage;

$numToGet = $numNewsPerPage + 1;


$query = "select first_post_id, subject from topics ".
    "where forum_id=$forumID order by first_post_id desc ".
    "limit $numToSkip, $numToGet;";

$result = mysql_query( $query );

$numRows = mysql_numrows( $result );

echo "<table border=0 width=100%>";

$numToShow = $numNewsPerPage;

if( $numToShow > $numRows ) {
    $numToShow = $numRows;
    }


for( $i=0; $i<$numToShow; $i++ ) {
    $subject = mysql_result( $result, $i, "subject" );
    $post_id = mysql_result( $result, $i, "first_post_id" );

    $query = "select posted, message from posts where id=$post_id;";

    $resultB = mysql_query( $query );
    $numRowsB = mysql_numrows( $resultB );

    if( $numRowsB == 1 ) {
        echo "<tr><td width=100%>";
        
        echo "<h3>$subject</h3>";

        $date = format_time( mysql_result( $resultB, 0, "posted" ) );
        $message = mysql_result( $resultB, 0, "message" );

        
        echo "$date<br>";

        echo "<pre>$message</pre>";
        
        echo "</td></tr>";
        }
    
    }





if( $pageNumber > 1 ) {
    // more left
    $prevPage = $pageNumber - 1;
    
    echo "<a href=news.php?numPerPage=$numNewsPerPage&page=$prevPage>Prev</a>";
    }


if( $numRows > $numNewsPerPage ) {
    // more left
    $nextPage = $pageNumber + 1;
    
    echo "<a href=news.php?numPerPage=$numNewsPerPage&page=$nextPage>Next</a>";
    }





mysql_close( $news_mysqlLink );



/**
 * Displays the operation error message and dies.
 *
 * @param $message the error message to display.
 */
function news_operationError( $message ) {
    
    // for now, just print error message
    echo( "ERROR:  $message" );
    die();
    }


/**
 * Filters a $_REQUEST variable using a regex match.
 *
 * Returns "" (or specified default value) if there is no match.
 */
function news_requestFilter( $inRequestVariable, $inRegex, $inDefault = "" ) {
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





?>