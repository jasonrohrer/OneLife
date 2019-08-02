<?php

// trick found here:
//  https://www.catswhocode.com/blog/how-to-create-a-simple-and-efficient-php-cache

// Cache the contents to a file
$cached = fopen( $cacheFile, 'w' );
fwrite( $cached, ob_get_contents() );
fclose( $cached );

// Send the output to the browser
ob_end_flush(); 
?>