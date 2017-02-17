<?php

include( "artLogSettings.php" );

global $passwordHashingPepper;

if( isset( $_REQUEST[ "password" ] ) ) {
    $hash = hash_hmac( "sha1", 
                       $_REQUEST[ "password" ],
                       $passwordHashingPepper );
    
    echo "Password hash is:  $hash";
    }
else {
?> 
    <FORM ACTION="passwordHashUtility_artLog.php" METHOD="post">
    <INPUT TYPE="password" MAXLENGTH=200 SIZE=20 NAME="password">
         <INPUT TYPE="Submit" VALUE="Generate Password Hash">

<?php
         
    }

?>
