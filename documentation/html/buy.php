<?php include( "header.php" ); ?>

<?php include( "reviewPages/reviewCount.php" ); ?>


<?php
function showPayLinks( $inSimple ) {
    $referrer = "";
    
    if( isset( $_SERVER['HTTP_REFERER'] ) ) {
        // pass it through without a regex filter
        // because we can't control its safety in the end anyway
        // (user can just edit URL sent to FastSpring).
        $referrer = urlencode( $_SERVER['HTTP_REFERER'] );
        }
    
    

    ?>
 <center>
      <center><table border=0><tr><td> 
<font size=3><ul> 
      <li>Lifetime server account
      <li>All future updates
      <li>Full source code
      <li>Tech support included
      </ul></font>
</td></tr></table>

      <font size=5>Available now for $20</font><br>
      <!--<font size=4 color=#ff0000>(25% off final price during launch week)</font><br><br>-->
      
      <a href="https://sites.fastspring.com/jasonrohrer/instant/onehouronelife?referrer=<?php echo $referrer;?>">
      <img src="fs_cards.png" width=280 height=45 border=0><?php
      if( !$inSimple ) {

          echo "<br>";
          
          echo "<img src=\"fs_button05.png\" width=210 height=60 border=0>";
          }
?>
      </a>
      </center>
<?php
    }

function showLogo( $inImageFile, $inText ) {

    echo "<table border=0><tr><td align=center>
          <img src=\"$inImageFile\"><br>
          <font size=1>$inText</font>
          </td></tr></table>";
    }

?>



<center>


<table border=0 width=100%>
<tr>
<td><img src=portraitLeft.jpg border=0 width=197 height=414></td>
<td>
<center><?php include( "lifeStats.php" ); ?></center>



<img src=logo558x305_border.jpg border=0 width=558 height=305>

<br>
<center>a multiplayer survival game of parenting<br>and civilization building by <a href=http://hcsoftware.sf.net/jason-rohrer>Jason Rohrer</a></center>

</td>
<td><img src=portraitRight.jpg border=0 width=196 height=418></td>

</tr>
</table>


 <center>
<table border=0><tr>
<td><?php showLogo( "noDRM.png", "No DRM" ); ?></td>
<td><?php showLogo( "noTie.png", "No middle-person" ); ?></td>
<td><?php showLogo( "crossPlatform.png", "Cross-platform" ); ?></td>
<td><?php showLogo( "openSource.png", "Open Source" ); ?></td>
</tr></table>                                  
</center>

<center>
<table border=0 cellpadding=2><tr><td bgcolor="#222222">
<table border=0 cellpadding=5><tr><td bgcolor="#000000">
<center> 

<?php
   showPayLinks( false );
?>

</td></tr></table>
</td></tr></table>
</center>
<br>
<br>
</center>

<font size=5>What you get</font><br>

<table border=0 width="100%" cellpadding=5><tr><td bgcolor="#222222">
<font size=3>
    Immediately after your payment is processed, you will receive an email with an access link.  You will then be able to download all of the following DRM-free distributions:
<center>
<table border=0><tr><td>
<font size=3><ul>
<li>Windows build</li>
<li>MacOS build (10.5 and later Intel)</li>
<li>GNU/Linux build (compiled on 32-bit Ubuntu 14.04)</li>
<li>Full source code bundle (compile it yourself)</li>
</ul></font>
</td></tr></table>
</center>
The price also includes downloads of all future updates and a lifetime account on the main game server that I am running.<br>
<br>
The source bundle includes the editor and server software, allowing you to set up and run your own server or even leverage the engine to make your own game.  See OneLife/documentation in the source bundle for instructions.<br>
<br>
You can take a look at the <a href="requirements.php">system requirements</a>.</font>
</td></tr></table>
<br>
<br>


     
    
<?php include( "footer.php" ); ?>