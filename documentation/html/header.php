<html>

<head>
<title>One Hour One Life</title>

<?php
global $pathToRoot;


$referrer = "";
    
if( isset( $_SERVER['HTTP_REFERER'] ) ) {
    // pass it through without a regex filter
    // because we can't control its safety in the end anyway
    // (user can just edit URL sent to FastSpring).
    $referrer = urlencode( $_SERVER['HTTP_REFERER'] );
    }
    

if( isset( $blockRobots ) && $blockRobots == 1 ) {
?>
    <meta name="robots" content="noindex, nofollow">
    <meta name="googlebot" content="noindex, nofollow">
<?php
    }
?>

</head>

<body bgcolor=#222222 text=white link=#b2a536 vlink=#b2a536 alink=#b2a536>


    
    <table border=0 cellspacing=5 cellpadding=0 width=100%><tr>

    <td align=center width=10%>[<a href='http://onehouronelife.com'>Home</a>]</td>
    <td align=center width=10%>[<a href='https://sites.fastspring.com/jasonrohrer/instant/onehouronelife?referrer=<?php echo $referrer;?>'>Buy</a>]</td>
    <td align=center width=10%>[<a href='http://onehouronelife.com/newsPage.php'>News</a>]</td>
    <td align=center width=10%>[<a href='http://onehouronelife.com/updateLog.php'>Update Log</a>]</td>
    <td align=center width=10%>[<a href='http://onehouronelife.com/foodStats.php'>Food Stats</a>]</td>
    <td align=center width=10%>[<a href='http://onehouronelife.com/failureStats.php'>Fail Stats</a>]</td>
    <td align=center width=10%>[<a href='https://onehouronelife.com/forums'>Forums</a>]</td>
    <td align=center width=10%>[<a href='http://onehouronelife.com/artLogPage.php'>Artwork</a>]</td>
    <td align=center width=10%>[<a href="http://onehouronelife.gamepedia.com">Wiki</a>]</td>
    <td align=center width=10%>[<a href='http://onehouronelife.com/credits.php'>Credits</a>]</td>


</tr></table>
<center>

<table border=0 width=100% bgcolor=black>
<tr>
<td align=center>
    <table border=0 width=900><tr><td>
