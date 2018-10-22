<?php

include( "forums/config.php" );

include_once( "newsCommon.php" );


// number id of the news forum to pull from
//$newsForumID = 4;
global $newsForumID;

// php file where READ MORE links should go
// $newsLinkPage = "newsPage.php"
global $newsLinkPage;



$news_mysqlLink =
    mysql_connect( $db_host, $db_username, $db_password )
    or news_operationError( "Could not connect to database server: " .
                            mysql_error() );

mysql_select_db( $db_name )
or news_operationError( "Could not select $databaseName database: " .
                        mysql_error() );




$pageNumber = news_requestFilter( "page", "/[0-9]+/", 1 );

global $numNewsPerPage;
global $newsSummaryOnly;

if( $numNewsPerPage < 1 ) {
    
    $numNewsPerPage = news_requestFilter( "numPerPage", "/[0-9]+/", -1 );
    }

if( $numNewsPerPage < 1 ) {
    $numNewsPerPage = 5;
    }


$postID = news_requestFilter( "postID", "/[0-9]+/", -1 );

$postSQL = "";

if( $postID != -1 ) {
    $postSQL = " and id=$postID ";
    }


    

$numToSkip = ( $pageNumber - 1 ) * $numNewsPerPage;

$numToGet = $numNewsPerPage + 1;


$query = "select id, num_replies, first_post_id, subject from topics ".
    "where forum_id=$newsForumID $postSQL order by first_post_id desc ".
    "limit $numToSkip, $numToGet;";

$result = mysql_query( $query );

$numRows = mysql_numrows( $result );

echo "<table border=0 width=100%>";

$numToShow = $numNewsPerPage;

if( $numToShow > $numRows ) {
    $numToShow = $numRows;
    }


for( $i=0; $i<$numToShow; $i++ ) {
    $topic_id = mysql_result( $result, $i, "id" );
    $subject = mysql_result( $result, $i, "subject" );
    $post_id = mysql_result( $result, $i, "first_post_id" );
    $num_replies = mysql_result( $result, $i, "num_replies" );

    $query =
        "select posted, message from posts where id=$post_id;";

    $resultB = mysql_query( $query );
    $numRowsB = mysql_numrows( $resultB );

    if( $numRowsB == 1 ) {
        echo "<tr><td width=100%>";
        
        echo "<font size=6>$subject</font><br>";
	
        $posted = mysql_result( $resultB, 0, "posted" );
        
        $date = date("F j, Y", $posted );
        $message = mysql_result( $resultB, 0, "message" );

        $firstReplyID = -1;

        if( $num_replies > 0 ) {
            $query =
                "select id from posts ".
                "where topic_id=$topic_id ".
                "order by id asc limit 1, 1;";
            
            $resultC = mysql_query( $query );
            $numRowsC = mysql_numrows( $resultC );

            if( $numRowsC > 0 ) {
                $firstReplyID = mysql_result( $resultC, 0, "id" );
                }
            }
        
        echo "$date<br><br>";
        
        $messageHTML = sb_rcb_blog2html( $message );

        //echo "<pre>$messageHTML</pre>";
        
        if( $newsSummaryOnly ) {
            if( strlen( $messageHTML ) > 500 ) {
                $breakPos = strpos( $messageHTML, "<br />", 500 );

                if( $breakPos > 500 ) {    
                    $messageHTML = substr( $messageHTML, 0, $breakPos );
                    }
                }
            
            echo "$messageHTML<br>";

            echo "[<a href=$newsLinkPage>Read More...</a>]<br>";
            }
        else {
            echo "$messageHTML<br>";

            $commentLinkText = "Comment";
            $commentLink = "forums/viewtopic.php?id=$topic_id";
            
            if( $num_replies > 0 && $firstReplyID > -1 ) {
                $commentLinkText = "$num_replies Comment";
                if( $num_replies > 1 ) {
                    $commentLinkText = $commentLinkText . "s";
                    }
                
                $commentLink =
                    "forums/viewtopic.php?id=$topic_id#p$firstReplyID";
                }

            echo "<table border=0 width=100%><tr><td align=left>".
                "[<a href=$newsLinkPage?postID=$topic_id>Link</a>]</td>".
                "<td align=right>".
                "[<a href=$commentLink>$commentLinkText</a>]".
                "</td></tr></table>";
            
            echo "<br><br><br><br><br><br>";
            }
        
        echo "</td></tr>";
        }
    }

    
echo "</table>";
    
if( ! $newsSummaryOnly ) {
        
    
    
    echo "<table boder=0 width=100%><tr>";
        
    echo "<td align=left>";
    
    
    if( $pageNumber > 1 ) {
        // more left
        $prevPage = $pageNumber - 1;
        
        echo "[<a href=$newsLinkPage?numPerPage=$numNewsPerPage&page=$prevPage>Prev</a>]";
        }
    
    echo "</td><td align=right>";
    
    if( $numRows > $numNewsPerPage ) {
        // more left
        $nextPage = $pageNumber + 1;
            
        echo "[<a href=$newsLinkPage?numPerPage=$numNewsPerPage&page=$nextPage>Next</a>]";
        }
    
    echo "</td></tr></table>";
    }
    


mysql_close( $news_mysqlLink );








?>