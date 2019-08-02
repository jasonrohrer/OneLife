
<?php

include( "settings.php" );


global $enableYubikey, $passwordHashingPepper;

?>


<script type="text/javascript" src="sha1.js"></script>

<script type="text/javascript">


  function calcHMAC() {
      try {
  
          var hmacInput = document.getElementById("hmacInputText");
          var pepperInput = document.getElementById("pepperInputText");
          var hmacOutput = document.getElementById("hmacOutputText");
  
          var shaObj = new jsSHA("SHA-1", "TEXT");
          shaObj.setHMACKey( pepperInput.value, "TEXT" );
          shaObj.update( hmacInput.value );
          hmacOutput.value = shaObj.getHMAC("HEX");

          } catch(e) {
              hmacOutput.value = e.message
          }
      }
</script>

<body onload="calcHMAC()">

<FORM>
    <INPUT TYPE="password" MAXLENGTH=20 SIZE=20 NAME="password" autofocus id="hmacInputText" onkeyup="calcHMAC()">
</FORM>



<FORM ACTION="server.php" METHOD="post">
<?php

if( $enableYubikey ) {
?>
    <br>
    Yubikey:<br>
    <INPUT TYPE="password" MAXLENGTH=48 SIZE=48 NAME="yubikey">

<?php
    }
?>

    <INPUT TYPE="hidden" NAME="action" VALUE="show_data">
	        <INPUT TYPE="Submit" VALUE="login">

<br>
<br>
Server-provided Pepper:
<br>
	<input type="text" size="75" name="pepper" readonly value="<?php echo $passwordHashingPepper;?>" id="pepperInputText">

    <br>
hmac_sha1 of password with pepper as key:<br>
	<input type="text" size="75" name="passwordHMAC" id="hmacOutputText">

    </FORM>


</body>