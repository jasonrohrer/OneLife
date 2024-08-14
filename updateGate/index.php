<?php

include( "settings.php" );

global $header, $footer;



eval( $header );

global $serialNumberFilePath;


$nextNumber = trim( file_get_contents( $serialNumberFilePath ) );

global $challengePrefix;

$challenge = $challengePrefix . $nextNumber;


echo "This will trigger a content update for Another Planet based on the latest content in the Github repository AnotherPlanetData.<br><br><br>";

echo "NOTE: this will only work if you are currently elected as ".
"Content Leader.<br><br><br>";


echo "Use game client SERVICES area to generate hash response:<br>";

global $fullServerURL;

?>
<center><br>
  <FORM ACTION="<?php echo $fullServerURL;?>" METHOD="post">
    <INPUT TYPE="hidden" NAME="action" VALUE="content_update">
    Challenge: <INPUT TYPE="text" MAXLENGTH=40 SIZE=20 NAME="challenge"
           VALUE="<?php echo $challenge;?>" readonly><br><br>
    Hash Response: <INPUT TYPE="text" MAXLENGTH=40 SIZE=40 NAME="response"
           VALUE=""><br><br>
    <INPUT TYPE="checkbox" NAME="wipe_map" VALUE=0> Wipe map<br><br>
    <INPUT TYPE="checkbox" NAME="wipe_map_confirm" VALUE=0> Confirm Wipe map<br><br>
    
    <INPUT TYPE="Submit" VALUE="Start Content Update">
  </FORM>
</center>
<hr>
Or:
<hr>

<br><br>

View the most recent (or current) content update log:<br>

<center><br>
  <FORM ACTION="<?php echo $fullServerURL;?>" METHOD="post">
    <INPUT TYPE="hidden" NAME="action" VALUE="view_log">
    Challenge: <INPUT TYPE="text" MAXLENGTH=40 SIZE=20 NAME="challenge"
           VALUE="<?php echo $challenge;?>" readonly><br><br>
    Hash Response: <INPUT TYPE="text" MAXLENGTH=40 SIZE=40 NAME="response"
           VALUE=""><br><br>
    <INPUT TYPE="Submit" VALUE="View Log">
  </FORM>
</center>
    

    
<?php




eval( $footer );




?>