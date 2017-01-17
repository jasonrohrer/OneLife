<?php

include( "requiredVersion.php" );

global $version;

// for now, keep reflector server as simple as possible

echo "192.168.1.124\n";
echo "8005\n";
echo "$version\n";
echo "http://192.168.1.124/jcr13/updateServer/server.php\n";
echo "OK";


?>