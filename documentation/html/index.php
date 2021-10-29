<?php include( "cacheTop.php" ); ?>

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
    
    if( $inSimple ) {
    ?>
      <a href="https://sites.fastspring.com/jasonrohrer/instant/onehouronelife?referrer=<?php echo $referrer;?>"><img src="fs_cards.png" width=280 height=45 border=0></a>

<?php
        return;
        }
?>    
        
 <center>
      <center><table border=0><tr><td> 
<font size=3><ul> 
      <li>Lifetime server account
      <li>All future updates
      <li>Unlock on Steam
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
<center><?php include( "lifeStats.php" ); ?><br>
<?php include( "monumentStats.php" ); ?><br>
<?php include( "arcServer/arcReport.php" ); ?>
</center>



<img src=logo558x305_border.jpg border=0 width=558 height=305>

<br>
<center>a multiplayer survival game of parenting<br>and civilization building by <a href=http://hcsoftware.sf.net/jason-rohrer>Jason Rohrer</a></center>

</td>
<td><img src=portraitRight.jpg border=0 width=196 height=418></td>

</tr>
</table>

     
<?php
if( $rs_reviewCount > 0 ) {
    $reviewWord = "Reviews";
    if( $rs_reviewCount == 1 ) {
        $reviewWord = "Review";
        }
?>

<a href="#reviews"><?php echo $rs_reviewCount;?> Player <?php echo $reviewWord;?></a>, <?php echo $rs_positivePercent;?>% Positive

<?php
    }
?>

<br>
<br>
<?php
   showPayLinks( true );
?>

<center><iframe title="YouTube video player" width="640" height="390" src="https://www.youtube.com/embed/mT4JktcVQuE?rel=0" frameborder="0" allowfullscreen></iframe></center>

<br>

    
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
<center><img src=lifeLine.jpg border=0 width=712 height=222></center>

<table border=0><tr><td align=justify>
<font size=5>This game is about playing one small part </font>in a much larger story.  You only live an hour, but time and space in this game is infinite.  You can only do so much in one lifetime, but the tech tree in this game will take hundreds of generations to fully explore.  This game is also about family trees.  Having a mother who takes care of you as a baby, and hopefully taking care of a baby yourself later in life.  And your mother is another player.  And your baby is another player.  Building something to use in your lifetime, but inevitably realizing that, in the end, what you build is not for YOU, but for your children and all the countless others that will come after you.  Proudly using your grandmother's ax, and then passing it on to your own grandchild as the end of your life nears.  And looking at each life as a unique story.  I was this kid born in this situation, but I eventually grew up.  I built a bakery near the wheat fields.  Over time, I watched my grandmother and mother grow old and die.  I had some kids of my own along the way, but they are grown now... and look at my character now!  She's an old woman.  What a life passed by in this little hour of mine.  After I die, this life will be over and gone forever.  I can be born again, but I can never live this unique story again.  Everything's changing.  I'll be born as a different person in a different place and different time, with another unique story to experience in the next hour...
</td></tr></table>

<center><img src=arrowLine.jpg border=0 width=712 height=219></center>

     
<br>
<br>
     

<table border=0 cellpadding=5><tr><td>     <font size=5>....Progress Report....</font>
     </td></tr></table>
     <table border=1 cellspacing=0 cellpadding=10 width=100%><tr><td>
<?php include( "objectsReport.php" ); ?>

     </td></tr></table>
<br>
<br>

<!--
<form action="http://northcountrynotes.org/releaseList/server.php" 
      method="post">
<input type="hidden" name="action" value="subscribe">
<input type="hidden" name="timeStamp" value="<?php echo file_get_contents( 'http://northcountrynotes.org/releaseList/server.php?action=timestamp' ); ?>">
Sign up for release announcement emails: <input type="text" name="email" value="">
<input type="submit" value="Subscribe"><br>
(A few brief emails a year, at most.)
</form>
<br>
-->

<center><iframe title="YouTube video player" width="640" height="390" src="https://www.youtube.com/embed/riqu2eszsIg?rel=0" frameborder="0" allowfullscreen></iframe></center>
    
<br><br>
    
<table border=0 width="640" cellpadding=10 cellspacing=0><tr>
    <td bgcolor="404040">
    <font size=5>What you get</font>
    </td></tr>
    <tr>
    <td bgcolor="#202020">
<font size=3>
    Immediately after your payment is processed, you will receive an email with an access link.  You will then be able to download all of the following DRM-free distributions:
<center>
<table border=0><tr><td>
<font size=3><ul>
<li>Windows build</li>
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

<center><iframe title="YouTube video player" width="640" height="390" src="https://www.youtube.com/embed/Hu7kXKuShks?rel=0" frameborder="0" allowfullscreen></iframe></center>   

<br>
<br>

     
</center>

     
<center>     


<table border=0 cellspacing=0>

<tr><td align=center colspan=3>
<font size=6 id="reviews"><?php echo $rs_positivePercent;?>% Positive Reviews:</font></td></tr>

  <tr>

<?php
if( $rs_reviewCount_positive > 0 ) {
?>


<td valign=top>
<font size=6>Recent Reviews:</font><br><br>
<?php
include( "reviewPages/recentReviewsPositive.html" );
?>
</td>

<?php
    }
if( $rs_reviewCount_positive > 16 ) {
?>
<td width=80></td>    
<td valign=top>
<font size=6>Top Playtime Reviews:</font><br><br>
<?php
include( "reviewPages/playtimeReviewsPositive.html" );
?>
</td>

<?php
    }
?>
      
</tr>
</table>

<br>
<br>


<table border=0 cellspacing=0>

<tr><td align=center colspan=3>
<font size=6><?php echo $rs_negativePercent;?>% Negative Reviews:</font></td></tr>

  <tr>

<?php
if( $rs_reviewCount_negative > 0 ) {
?>


<td valign=top>
<font size=6>Recent Reviews:</font><br><br>
<?php
include( "reviewPages/recentReviewsNegative.html" );
?>
</td>

<?php
    }
if( $rs_reviewCount_negative > 16 ) {
?>
<td width=80></td>    
<td valign=top>
<font size=6>Top Playtime Reviews:</font><br><br>
<?php
include( "reviewPages/playtimeReviewsNegative.html" );
?>
</td>

<?php
    }
?>
      
</tr>
</table>




</center>

<br>

<br>

<br>
     

<?php 
$artSummaryOnly = 1;
$numArtPerPage = 1;

include( "artLog.php" );
echo "<center>[<a href=artLogPage.php>More Artwork...</a>]</center>";
?>

<br><br><br>

     
<?php

$numNewsPerPage = 1;
$newsSummaryOnly = 1;
$newsForumID = 4;
$newsLinkPage = "newsPage.php";
include( "news.php" );

?>
<br>

<br>

<br>

<br>



<?php

$numNewsPerPage = 1;
$newsSummaryOnly = 1;
$newsForumID = 8;
$newsLinkPage = "fanArtPage.php";
include( "news.php" );

?>
<br>

<br>

<br>

<br>


<?php

$numNewsPerPage = 1;
$newsSummaryOnly = 1;
$newsForumID = 9;
$newsLinkPage = "userStoriesPage.php";
include( "news.php" );

?>
<br>

<br>

<br>

<br>



<center>
<font size=5>The thinking behind One Hour One Life</font><br>

<?php include( "youTubePlaylist.php" ); ?>


    <br>
    <br>
    <br>
    <br>

    <iframe src="https://discordapp.com/widget?id=328215300279500800&theme=dark" width="480" height="320" allowtransparency="true" frameborder="0"></iframe>

</center>


    
<?php include( "footer.php" ); ?>

    
<?php include( "cacheBottom.php" ); ?>

    