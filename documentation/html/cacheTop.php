<?php

// trick found here:
//  https://www.catswhocode.com/blog/how-to-create-a-simple-and-efficient-php-cache


$url = $_SERVER[ "SCRIPT_NAME" ];
$break = explode( '/', $url );
$file = $break[ count( $break ) - 1 ];
$cachefile = '/tmp/cached-' . substr_replace( $file , "", -4 ) . '.html';
$cachetime = 120;

// Serve from the cache if it is younger than $cachetime
if( file_exists( $cachefile ) &&
    time() - $cachetime < filemtime( $cachefile ) ) {

    $fileTime = filemtime( $cachefile );
    $timeLeft = ( time() - $fileTime ) - $cachetime;
    
    echo "<!-- Cached copy, generated " .
        date( 'H:i:s', filemtime( $cachefile ) ) .
        ", $timeLeft sec remain until regen -->\n";

    include( $cachefile );
    exit;
    }
// Start the output buffer
ob_start();

// cacheBottom will handle saving the contents of the ouptput buffer.
?>