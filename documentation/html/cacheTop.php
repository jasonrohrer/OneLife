<?php

// trick found here:
//  https://www.catswhocode.com/blog/how-to-create-a-simple-and-efficient-php-cache


$url = $_SERVER[ "SCRIPT_NAME" ];
$break = explode( '/', $url );
$file = $break[ count( $break ) - 1 ];
$cacheFile = '/tmp/cached-' . substr_replace( $file , "", -4 ) . '.html';
$cacheTime = 120;

// Serve from the cache if it is younger than $cacheTime
if( file_exists( $cacheFile ) &&
    time() - $cacheTime < filemtime( $cacheFile ) ) {

    $fileTime = filemtime( $cacheFile );
    $timeLeft = $cacheTime - ( time() - $fileTime );
    
    echo "<!-- Cached copy, generated " .
        date( 'H:i:s', filemtime( $cacheFile ) ) .
        ", $timeLeft sec remain until regen -->\n";

    include( $cacheFile );
    exit;
    }
// Start the output buffer
ob_start();

// cacheBottom will handle saving the contents of the ouptput buffer.
?>