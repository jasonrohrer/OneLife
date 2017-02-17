<?php include( "header.php" ); ?>

<center>



<br>
<br>

<img border=0 width=448 height=360 src="logo448x360.png">
<br><br>
<br>
a multiplayer survival game of parenting and civilization building by <a href=http://hcsoftware.sf.net/jason-rohrer>Jason Rohrer</a> and <a href=https://soundcloud.com/delcroy>Tom Bailey</a>

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
     

<iframe title="YouTube video player" width="640" height="390" src="http://www.youtube.com/embed/uYjm3XMrqNo?rel=0" frameborder="0" allowfullscreen></iframe>
<br>
<br>

<form action="http://northcountrynotes.org/releaseList/server.php" 
      method="post">
<input type="hidden" name="action" value="subscribe">
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


<?php include( "footer.php" ); ?>
