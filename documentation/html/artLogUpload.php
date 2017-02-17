<?php
include( "header.php" );


include( "artLogSettings.php" );











$files = array();
$dir = opendir('artPosts');

if( $dir === false ) {
    echo "Report failed\n";
    exit();
    }




while( false !== ( $file = readdir($dir) ) ) {
    if( ($file != ".") and ($file != "..") ) {
        $files[] = $file;
        }   
    }

natsort( $files );

$files = array_reverse( $files );

$numFiles = sizeof( $files );


$upload = 0;
if( isset( $_REQUEST[ "upload" ] ) ) {
    $upload = $_REQUEST[ "upload" ];
    }


if( !$upload ) {
    // show form

global $passwordHashingPepper;

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
      Password:
    <INPUT TYPE="password" MAXLENGTH=20 SIZE=20 NAME="password" autofocus id="hmacInputText" onkeyup="calcHMAC()">
</FORM>



<FORM enctype="multipart/form-data" ACTION="artLogUpload.php" METHOD="post">
    
      
    <INPUT TYPE="hidden" NAME="upload" VALUE="1">

<br>
<br>
Server-provided Pepper:
<br>
	<input type="text" size="75" name="pepper" readonly value="<?php echo $passwordHashingPepper;?>" id="pepperInputText">

    <br>
hmac_sha1 of password with pepper as key:<br>
	<input type="text" size="75" name="passwordHMAC" id="hmacOutputText">

      <br><br><br><br>
      Post this image file: <input name="userfile" type="file" />
      <br>
      <br>
       <INPUT TYPE="Submit" VALUE="Upload">

      
    </FORM>


</body>


<?php
      
    }
else {
    // trying to upload

    global $accessPasswords, $passwordHashingPepper;

    $loginPermitted = false;
    
    if( isset( $_REQUEST[ "passwordHMAC" ] ) ) {
        $password = $_REQUEST[ "passwordHMAC" ];
        
        if( in_array( $password, $accessPasswords ) ) {
            $loginPermitted = true;
            }
        }
    
    if( !$loginPermitted ) {
        echo "Login failed";
        die();
        }

    if( ! isset( $_FILES['userfile']['tmp_name'] ) ) {
        echo "Upload failed";
        die();
        }

    
    
    $tmpFileName = $_FILES['userfile']['tmp_name'];

    $originalFileName = $_FILES['userfile']['name'];
    
    $ext = pathinfo( $originalFileName, PATHINFO_EXTENSION );

    if( $ext != "jpg" && $ext != "png" ) {
        echo "Only JPG and PNG files supported.";
        die();
        }
    
    $lastFileNumber = 0;
    
    if( $numFiles > 0 ) {
        $lastFileNumber = pathinfo( $files[0], PATHINFO_FILENAME );
        }
    
    $newFileNumber = $lastFileNumber + 1;

    $path =
        substr( $_SERVER["SCRIPT_FILENAME"],
                0, strpos( $_SERVER["SCRIPT_FILENAME"], "/artLogUpload.php" ) );
    
    $bigFile = "artPostsBig/$newFileNumber.$ext";

    echo "Renaming $tmpFileName to $bigFile <br>";
    
    rename( $tmpFileName, $bigFile );

    $postFile = "artPosts/$newFileNumber.jpg";

    shell_exec( "convert -resize '600>' -quality 92 $bigFile $postFile" );

    echo "Converting to small display version at $postFile<br>";
    
    echo "Upload complete.<br><br>";
    
    
    echo "<a href=artLogPage.php>Go to art log</a>";
    
    }



?>