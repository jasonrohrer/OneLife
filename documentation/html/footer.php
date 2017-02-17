
</td>
</tr>
</table>

</td>
</tr>
</table>
</center>

<?php

$nocounter = "0";
if( isset( $_REQUEST[ "nocounter" ] ) ) {
    $nocounter = $_REQUEST[ "nocounter" ];
    }

if( ! $nocounter && ( !isSet( $blockCounter ) || ! $blockCounter ) ) {
?>

<script src="//static.getclicky.com/js" type="text/javascript"></script>
<script type="text/javascript">try{ clicky.init(101006849); }catch(e){}</script>
<noscript><p><img alt="Clicky" width="1" height="1" src="//in.getclicky.com/101006849ns.gif" /></p></noscript> 

<?php
     }
?>

     
</center>


</body>

</html>