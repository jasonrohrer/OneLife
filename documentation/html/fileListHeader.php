// backup of current text
// goes in ticketServer/settings.php


$fileListHeader = $header .
'echo "<center><font size=6>Downloads</font><br><br>"; ' .
'echo "Your account email is: <b>$email</b><br><br>"; '.
'echo "Your Account Key is: <b>$ticket_id</b><br><br><br>"; '.
'$email_sha1 = sha1( strtolower( $email ) );'.
'$pureTicket = strtoupper( implode( preg_split( "/-/", $ticket_id ) ) );'.
'$stringToHash = mt_rand( 0, 2000000000 );'.
'$ticket_hash = strtoupper( ts_hmac_sha1( $pureTicket, $stringToHash ) );'.    
'echo "<FORM ACTION=http://lineage.onehouronelife.com/server.php METHOD=post><INPUT TYPE=hidden NAME=action VALUE=front_page><INPUT TYPE=hidden NAME=hide_filter VALUE=1><INPUT TYPE=hidden NAME=email_sha1 VALUE=$email_sha1><INPUT TYPE=hidden NAME=ticket_hash VALUE=$ticket_hash><INPUT TYPE=hidden NAME=string_to_hash VALUE=$stringToHash>See your recent lives in the <INPUT TYPE=Submit VALUE=\"Family Tree\"></FORM><br>"; '.
'echo "You can also <a href=http://onehouronelife.com/steamGate/server.php?action=unlock_on_steam&ticket_id=$ticket_id>unlock the game on Steam</a><br><br><br>"; '.
'echo "To register for the forums, use this secret phrase: <b>SECRET_CODE</b><br><br><br>"; '.
'if( $coupon_code != "" ) { '.
'    echo "Give this coupon code to a friend: "; '.
'    echo "<b>$coupon_code</b><br><br><br>"; '.
'    } '.
'echo "</center>"; ';
