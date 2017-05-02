<?php

include( "header.php" );


include( "artLogSettings.php" );










$files = array();
$thumbs = array();
$numFiles = 0;
$numThumbs = 0;



function rescanFiles() {
    global $files, $thumbs, $numThumbs, $numFiles;

    $files = array();
    $thumbs = array();
    
    $dir = opendir('artPosts');

    if( $dir === false ) {
        echo "Report failed\n";
        exit();
        }    

    while( false !== ( $file = readdir($dir) ) ) {
        if( ($file != ".") and ($file != "..") ) {
            if( ! preg_match( "/[0-9]+_t.jpg/", $file ) ) {
                $files[] = $file;
                }
            else {
                $thumbs[] = $file;
                }
            }   
        }

    natsort( $files );
    natsort( $thumbs );
    
    $files = array_reverse( $files );
    $thumbs = array_reverse( $thumbs );

    $numFiles = sizeof( $files );
    $numThumbs = sizeof( $thumbs );
    }


rescanFiles();



$upload = 0;
if( isset( $_REQUEST[ "upload" ] ) ) {
    $upload = $_REQUEST[ "upload" ];
    }



global $passwordHashingPepper;


echo "<body>";


if( $upload ) {
        
    // trying to upload

    global $accessPasswords, $passwordHashingPepper;
    
    $loginPermitted = false;

    $passwordSubmittedHash = "";
    
    if( isset( $_REQUEST[ "passwordHMAC" ] ) ) {
        $passwordSubmittedHash = $_REQUEST[ "passwordHMAC" ];

        preg_match( "/[0-9a-f]+/", $passwordSubmittedHash, $matches );
        $passwordSubmittedHash = $matches[0];

        
        
        $passwordDoubleHash = hash_hmac( "sha1",
                                         $passwordSubmittedHash,
                                         $passwordHashingPepper );
        
        if( in_array( $passwordDoubleHash, $accessPasswords ) ) {
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


    $lastFileNumber = 0;
    
    if( $numFiles > 0 ) {
        $lastFileNumber = pathinfo( $files[0], PATHINFO_FILENAME );
        }
    
    $newTop = $_REQUEST[ "newTop" ];
    
    preg_match( "/[0-9]+/", $newTop, $matches );
    $newTop = $matches[0];

    $deleteMarked = $_REQUEST[ "deleteMarked" ];
    
    preg_match( "/[01]/", $deleteMarked, $matches );
    $deleteMarked = $matches[0];
    
    if( $newTop > 0 ) {

        if( $deleteMarked == 1 ) {
            unlink( "artPosts/$newTop.jpg" );
            unlink( "artPosts/$newTop"."_t.jpg" );
        
        
            if( file_exists( "artPostsBig/$newTop.jpg" ) ) {
                unlink( "artPostsBig/$newTop.jpg" );
                }
        
            if( file_exists( "artPostsBig/$newTop.png" ) ) {
                unlink( "artPostsBig/$newTop.png" );
                }
            if( $newTop == $lastFileNumber ) {
                if( $numFiles > 1 ) {
                    $lastFileNumber = pathinfo( $files[0], PATHINFO_FILENAME );
                    }
                else {
                     $lastFileNumber = 0;
                    }
                }
            }
        else if( $newTop < $lastFileNumber ) {
            $newTopNumber = $lastFileNumber + 1;
        
            rename( "artPosts/$newTop.jpg", "artPosts/$newTopNumber.jpg" );
            rename( "artPosts/$newTop"."_t.jpg",
                    "artPosts/$newTopNumber"."_t.jpg" );
        
        
            if( file_exists( "artPostsBig/$newTop.jpg" ) ) {
                rename( "artPostsBig/$newTop.jpg",
                        "artPostsBig/$newTopNumber.jpg" );
                }
        
            if( file_exists( "artPostsBig/$newTop.png" ) ) {
                rename( "artPostsBig/$newTop.png",
                        "artPostsBig/$newTopNumber.png" );
                }
        
            $lastFileNumber = $newTopNumber;
            }
        }
    


    
    
    $tmpFileName = $_FILES['userfile']['tmp_name'];

    $originalFileName = $_FILES['userfile']['name'];
    
    $ext = pathinfo( $originalFileName, PATHINFO_EXTENSION );

    if( $ext != "" && 
        $ext != "jpg" && $ext != "png" ) {

        echo "Only JPG and PNG files supported, not '$ext'";
        }
    else if( $ext != "" ) {    
    
        $newFileNumber = $lastFileNumber + 1;

        $path =
            substr( $_SERVER["SCRIPT_FILENAME"],
                    0, strpos( $_SERVER["SCRIPT_FILENAME"],
                               "/artLogUpload.php" ) );
    
        $bigFile = "artPostsBig/$newFileNumber.$ext";

        echo "Renaming $tmpFileName to $bigFile <br>";
    
        rename( $tmpFileName, $bigFile );
        chmod( $bigFile, 0666 );

        $postFile = "artPosts/$newFileNumber.jpg";
        $postFileThumb = "artPosts/$newFileNumber"."_t.jpg";

        shell_exec( "convert -resize '600>' -quality 92 $bigFile $postFile" );
        shell_exec(
            "convert -resize '80>' -quality 92 $bigFile $postFileThumb" );

        echo "Converting to small display versions at ".
            "$postFile, $postFileThumb<br>";

        chmod( $postFile, 0666 );
        chmod( $postFileThumb, 0666 );
    
        echo "Upload complete.<br><br>";
        
        }
    
    rescanFiles();
    }



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


<?php

$hashValueString = "";
$passwordFill = "";

if( isset( $_REQUEST[ "passwordHMAC" ] ) ) {
    $passwordSubmittedHash = $_REQUEST[ "passwordHMAC" ];
    
    preg_match( "/[0-9a-f]+/", $passwordSubmittedHash, $matches );
    $passwordSubmittedHash = $matches[0];

    $hashValueString = "value='$passwordSubmittedHash'";
    $passwordFill = "aaaaaaaaaa";
    }

?>


<FORM>
      Password:
<INPUT TYPE="password" MAXLENGTH=20 SIZE=20 NAME="password" autofocus id="hmacInputText" onkeyup="calcHMAC()" value=<?php echo $passwordFill;?> >
</FORM>



<FORM enctype="multipart/form-data" ACTION="artLogUpload.php" METHOD="post">


  Pick Top Image:<br>
  <table border=1 cellpadding=5 cellspacing=0>
    <tr>
<?php

      for( $i=0; $i<$numThumbs; $i++ ) {
          $t = $thumbs[$i];

          preg_match( "/([0-9]+)_t.jpg/", $t, $matches );

          $num = $matches[1];
          
          echo "<td align=center valign=bottom><img src='artPosts/$t'><br>\n";
          $checked = "";
          if( $i == 0 ) {
              $checked = "checked";
              }
          echo "<input type=radio name=newTop value=$num $checked>\n";
          echo "</td>\n";

          if( $i > 0 && $i % 6 == 0 ) {
              echo "</tr><tr>";
              }
          }

    if( $numThumbs > 0 ) {
        echo "</tr><tr><td colspan=6><input type=checkbox name=deleteMarked value=1> Delete Marked</td>";
        }

?>

</tr></table>

    <INPUT TYPE="hidden" NAME="upload" VALUE="1">

<br>
<br>
Server-provided Pepper:
<br>
	<input type="text" size="75" name="pepper" readonly value="<?php echo $passwordHashingPepper;?>" id="pepperInputText">

    <br>
hmac_sha1 of password with pepper as key:<br>
	<input type="text" size="75" name="passwordHMAC" 
		   readonly id="hmacOutputText" <?php echo $hashValueString;?> >

      <br><br><br><br>
      Post this image file: <input name="userfile" type="file" />
      <br>
      <br>
       <INPUT TYPE="Submit" VALUE="Update">

      
    </FORM>


</body>


<?php
      
    


?>
