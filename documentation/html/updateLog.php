<?php include( "header.php" ); ?>

<?php




$numPerPage = 5;



$page = "0";
if( isset( $_REQUEST[ "page" ] ) ) {
    $page = $_REQUEST[ "page" ];
    }


$numToSkip = $page * $numPerPage;


$files = array();
$dir = opendir('updatePosts');

if( $dir === false ) {
    echo "Report failed\n";
    exit();
    }




while( false !== ( $file = readdir($dir) ) ) {
    if( ($file != ".") and ($file != "..") ) {
        $files[] = $file;
        }   
    }

natsort( $files );

$files = array_reverse( $files );

$numFiles = sizeof( $files );


echo
"<table width=100% border=0><tr><td align=left>[<a href=.>Home</a>]</td><td align=center><font size=5>One Hour One Life - Content Update Log</font></td><td></td></tr></table>";
echo "<br><br><br>";



echo "<center><table border=0 width=400><tr><td>";


// forward/back links at top?
if( $numToSkip > 0 ) {
    
    echo "<table border=0 width=100%><tr><td align=left>";


    if( $numToSkip > 0 ) {
        $prevPage = $page - 1;
        
        echo "[[<a href=\"updateLog.php?page=$prevPage\">Newer</a>]]";
        }
    echo "</td><td align=right>";
    
    
    if( $numToSkip + $numPerPage < $numFiles ) {
        $nextPage = $page + 1;
        
        echo "[[<a href=\"updateLog.php?page=$nextPage\">Older</a>]]";
        }
    echo "</td></tr></table>";
    }




$shown=0;

for( $i=$numToSkip; $shown<$numPerPage && $i<$numFiles; $i++ ) {
    
    $file = $files[$i];

    $filePath = "updatePosts/$file";

    include( "$filePath" );
    echo "<br><br><br><br>";
    $shown++;
    }



echo "<table border=0 width=100%><tr><td align=left>";


if( $numToSkip > 0 ) {
    $prevPage = $page - 1;
    
    echo "[[<a href=\"updateLog.php?page=$prevPage\">Newer</a>]]";
    }
echo "</td><td align=right>";


if( $numToSkip + $numPerPage < $numFiles ) {
    $nextPage = $page + 1;
    
    echo "[[<a href=\"updateLog.php?page=$nextPage\">Older</a>]]";
    }
echo "</td></tr></table>";


echo "</td></tr></table></center>";




?>

<?php include( "footer.php" ); ?>
