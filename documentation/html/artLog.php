<?php include( "header.php" ); ?>

<?php




$numPerPage = 2;



$page = "0";
if( isset( $_REQUEST[ "page" ] ) ) {
    $page = $_REQUEST[ "page" ];
    }


$numToSkip = $page * $numPerPage;


$files = array();
$dir = opendir('artPosts');

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
"<table width=100% border=0><tr><td align=right>[<a href=.>Home</a>]</td></tr></table>";
echo "<br><br><br>";



echo "<center><table border=0 width=400><tr><td>";


// forward/back links at top?
if( $numToSkip > 0 ) {
    
    echo "<table border=0 width=100%><tr><td align=left>";


    if( $numToSkip > 0 ) {
        $prevPage = $page - 1;
        
        echo "[<a href=\"artLog.php?page=$prevPage\">Newer</a>]";
        }
    echo "</td><td align=right>";
    
    
    if( $numToSkip + $numPerPage < $numFiles ) {
        $nextPage = $page + 1;
        
        echo "[<a href=\"artLog.php?page=$nextPage\">Older</a>]";
        }
    echo "</td></tr></table>";
    }




$shown=0;

for( $i=$numToSkip; $shown<$numPerPage && $i<$numFiles; $i++ ) {
    
    $file = $files[$i];

    $filePath = "artPosts/$file";
    $bigFilePath = "artPostsBig/$file";

    $link = false;
    
    if( file_exists( $bigFilePath ) ) {
        $link = true;
        }

    if( $link ) {
        echo "<a href=$bigFilePath>";
        }
    
    echo "<img border=0 src=$filePath>";

    if( $link ) {
        echo "</a>";
        }
    echo "<br><br><br><br>";
    $shown++;
    }



echo "<table border=0 width=100%><tr><td align=left>";


if( $numToSkip > 0 ) {
    $prevPage = $page - 1;
    
    echo "[<a href=\"artLog.php?page=$prevPage\">Newer</a>]";
    }
echo "</td><td align=right>";


if( $numToSkip + $numPerPage < $numFiles ) {
    $nextPage = $page + 1;
    
    echo "[<a href=\"artLog.php?page=$nextPage\">Older</a>]";
    }
echo "</td></tr></table>";


echo "</td></tr></table></center>";




?>

<?php include( "footer.php" ); ?>
