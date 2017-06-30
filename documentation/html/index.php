<?php include( "header.php" ); ?>

<center>



<br>
<br>


<img src=logo558x305_border.png border=0>
<br><br>
<br>
a multiplayer survival game of parenting and civilization building by <a href=http://hcsoftware.sf.net/jason-rohrer>Jason Rohrer</a>

<br>
<br>
<br>
<br>
     

<table border=0 cellpadding=5><tr><td>     <font size=5>....Progress Report....</font>
     </td></tr></table>
     <table border=1 cellspacing=0 cellpadding=10 width=100%><tr><td>
<?php include( "objectsReport.php" ); ?>

     </td></tr></table>
<br>
<br>
     

<br>
<br>

<form action="http://northcountrynotes.org/releaseList/server.php" 
      method="post">
<input type="hidden" name="action" value="subscribe">
<input type="hidden" name="timeStamp" value="<?php echo file_get_contents( 'http://northcountrynotes.org/releaseList/server.php?action=timestamp' ); ?>">
Sign up for release announcement emails: <input type="text" name="email" value="">
<input type="submit" value="Subscribe"><br>
(A few brief emails a year, at most.)
</form>
<br>

<br>

</center>
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
