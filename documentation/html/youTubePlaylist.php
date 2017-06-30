<style>
.vid-list-element {
    background-color: rgb( 16, 16, 16 );
   padding:5px 5px 5px 15px;
         font-size:11px;
         }

.pic-container {
   width: 200px;
   height: 390px;
     overflow-y: scroll;
    overflow-x:hidden;
    }
div.clear {
  clear: both;
  overflow: hidden;
  height: 0;
    }


</style>



<script type="text/javascript">

function mouseVid( videoID ) {
    if( !document.getElementById( videoID ).myPressed ) {
        document.getElementById( videoID ).style.backgroundColor = "#222222";
    }
}

function showVid( videoID ) {
    //alert( "hey" );
    document.getElementById( videoID ).style.backgroundColor = "#444444";
    document.getElementById( videoID ).myPressed = 1;
    
    document.getElementById("playerInner").src =
		"https://www.youtube.com/embed/" + videoID + "?rel=0&autoplay=1";
}
</script>




<?php


// regular file_get_contents changed https support in PHP 5.6
// easier to use curl
function file_get_contents_curl( $url ) {
    $ch = curl_init();
    curl_setopt( $ch, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4 );
    curl_setopt( $ch, CURLOPT_AUTOREFERER, TRUE );
    curl_setopt( $ch, CURLOPT_HEADER, 0 );
    curl_setopt( $ch, CURLOPT_RETURNTRANSFER, 1 );
    curl_setopt( $ch, CURLOPT_URL, $url );
    curl_setopt( $ch, CURLOPT_FOLLOWLOCATION, TRUE );
    $data = curl_exec( $ch );
    curl_close( $ch );

    return $data;
    }

$apiURL = "https://www.googleapis.com/youtube/v3/playlistItems".
    "?playlistId=PLGRmlIh53cOLvQTajDxlPVcKEMloipoOE".
    "&part=contentDetails,snippet&maxResults=50".
    "&key=AIzaSyDB_BV1KJCtpI3JCkdxjER3NRYtOlc9jqc";
    
$list = file_get_contents_curl( $apiURL );


$videoIDs = array();
$titles = array();

foreach( preg_split("/((\r?\n)|(\r\n?))/", $list ) as $line ){
    if( preg_match( "/\"title\":/", $line ) ) {
        preg_match( "/\"title\":\s*\"([^\"]+)\"/", $line, $matches );
        $titles[] = $matches[1];
        }
    if( preg_match( "/\"videoId\":/", $line ) ) {
        preg_match( "/\"videoId\":\s*\"([^\"]+)\"/", $line, $matches );
        
        if( ! in_array( $matches[1], $videoIDs ) ) {
            $videoIDs[] = $matches[1];
            }
        }
    }


$firstVid = "uYjm3XMrqNo";

if( count( $videoIDs ) > 0 ) {
    $firstVid = $videoIDs[0];
    }

echo "<script type=\"text/javascript\">function clearAllMarks(inForce) {\n";

for( $i=0; $i < count( $videoIDs ); $i++ ) {
    $v = $videoIDs[$i];
    echo "if( inForce || ! document.getElementById( '$v' ).myPressed ) {\n";
    echo "    document.getElementById( '$v' ).style.backgroundColor = ".
        "'rgb(16,16,16)';\n    }\n";
    echo "if( inForce ) document.getElementById( '$v' ).myPressed = 0;\n";
    }
echo "}</script>\n\n";

?>

  <div id=playerAndMenu style=width:840px>

  <div id=playerArea style=float:left>
	<iframe id=playerInner title="YouTube video player" 
			width="640" height="390" 
			src="https://www.youtube.com/embed/<?php echo $firstVid;?>?rel=0" 
			frameborder="0" allowfullscreen></iframe>
  </div>


<?php
       
echo "<div id=playerMenu style=float:right class=\"pic-container\">\n";

for( $i=0; $i < count( $videoIDs ); $i++ ) {
    $t = $titles[$i];
    $v = $videoIDs[$i];
    echo "<div id='$v' class='vid-list-element' ".
        "onclick='clearAllMarks(1); showVid( \"$v\" )' ".
        "onmouseout='clearAllMarks(0)' ".
        "onmouseover='clearAllMarks(0); mouseVid( \"$v\" )'>".
        "$t<br><img src=https://i.ytimg.com/vi/$v/default.jpg></div>\n";
    }


echo "</div>\n";




?>
    


</div>

<div class="clear"></div>
