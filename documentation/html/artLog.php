
<?php



global $numArtPerPage, $artSummaryOnly;


$page = "0";
if( isset( $_REQUEST[ "page" ] ) ) {
    $page = $_REQUEST[ "page" ];
    }


$numToSkip = $page * $numArtPerPage;


$files = array();
$dir = opendir('artPosts');

if( $dir === false ) {
    echo "Report failed\n";
    exit();
    }




while( false !== ( $file = readdir($dir) ) ) {
    if( ($file != ".") and ($file != "..") and
        preg_match( "/[0-9]+.jpg/", $file ) and
        ! preg_match( "/[0-9]+_t.jpg/", $file ) ) {
        $files[] = $file;
        }   
    }

natsort( $files );

$files = array_reverse( $files );

$numFiles = sizeof( $files );


echo "<center><table border=0 width=400><tr><td align=center>";


// forward/back links at top?
if( !$artSummaryOnly && $numToSkip > 0 ) {
    
    echo "<table border=0 width=100%><tr><td align=left>";


    if( $numToSkip > 0 ) {
        $prevPage = $page - 1;
        
        echo "[<a href=\"artLogPage.php?page=$prevPage\">Newer</a>]";
        }
    echo "</td><td align=right>";
    
    
    if( $numToSkip + $numArtPerPage < $numFiles ) {
        $nextPage = $page + 1;
        
        echo "[<a href=\"artLogPage.php?page=$nextPage\">Older</a>]";
        }
    echo "</td></tr></table>";
    }




$shown=0;

for( $i=$numToSkip; $shown<$numArtPerPage && $i<$numFiles; $i++ ) {
    
    $file = $files[$i];

    $filePath = "artPosts/$file";
    $bigFilePath = "artPostsBig/$file";

    $link = false;


    if( ! file_exists( $bigFilePath ) ) {
        $name = pathinfo( $file, PATHINFO_FILENAME );

        // try png for bigfile
        $bigFilePath = "artPostsBig/$name.png";
        }
    
    if( file_exists( $bigFilePath ) ) {
        $link = true;
        }

    if( $link && !$artSummaryOnly ) {
        echo "<a href=$bigFilePath>";
        }
    else if( $artSummaryOnly ) {
        $link = true;
        echo "<a href=artLogPage.php>";
        }
    
    echo "<img border=0 src=$filePath>";

    if( $link ) {
        echo "</a>";
        }
    if( ! $artSummaryOnly ) {
        echo "<br><br><br><br>";
        }
    $shown++;
    }



if( ! $artSummaryOnly ) {
    
    echo "<table border=0 width=100%><tr><td align=left>";

    
    if( $numToSkip > 0 ) {
        $prevPage = $page - 1;
        
        echo "[<a href=\"artLogPage.php?page=$prevPage\">Newer</a>]";
        }
    echo "</td><td align=right>";
    
    
    if( $numToSkip + $numArtPerPage < $numFiles ) {
        $nextPage = $page + 1;
        
        echo "[<a href=\"artLogPage.php?page=$nextPage\">Older</a>]";
        }
    echo "</td></tr></table>";
    }


echo "</td></tr></table></center>";




?>
