<?php

/**
 * Displays the operation error message and dies.
 *
 * @param $message the error message to display.
 */
function news_operationError( $message ) {
    
    // for now, just print error message
    echo( "ERROR:  $message" );
    die();
    }


/**
 * Filters a $_REQUEST variable using a regex match.
 *
 * Returns "" (or specified default value) if there is no match.
 */
function news_requestFilter( $inRequestVariable, $inRegex, $inDefault = "" ) {
    if( ! isset( $_REQUEST[ $inRequestVariable ] ) ) {
        return $inDefault;
        }
    
    $numMatches = preg_match( $inRegex,
                              $_REQUEST[ $inRequestVariable ], $matches );

    if( $numMatches != 1 ) {
        return $inDefault;
        }
        
    return $matches[0];
    }



/**
 * Renders text containing BBCode as HTML for presentation in a browser.
 *
 * This function written by Noah Medling <noah.medling@gmail.com> as part
 * of RCBlog.
 *
 * @param $inData the data to convert.
 *
 * @return the stripped data.
 */
function sb_rcb_blog2html( $inData ){
    $patterns = array(
        "@(\r\n|\r|\n)?\\[\\*\\](\r\n|\r|\n)?(.*?)(?=(\\[\\*\\])|(\\[/list\\]))@si",
			
        // [b][/b], [i][/i], [u][/u], [mono][/mono]
        "@\\[b\\](.*?)\\[/b\\]@si",
        "@\\[i\\](.*?)\\[/i\\]@si",
        "@\\[u\\](.*?)\\[/u\\]@si",
        "@\\[mono\\](.*?)\\[/mono\\]@si",
			
        // [color=][/color], [size=][/size]
        "@\\[color=([^\\]\r\n]*)\\](.*?)\\[/color\\]@si",
        "@\\[size=([0-9]+)\\](.*?)\\[/size\\]@si",
			
        // [quote=][/quote], [quote][/quote], [code][/code]
        "@\\[quote=&quot;([^\r\n]*)&quot;\\](\r\n|\r|\n)*(.*?)(\r\n|\r|\n)*\\[/quote\\](\r\n|\r|\n)?@si",
        "@\\[quote\\](\r\n|\r|\n)*(.*?)(\r\n|\r|\n)*\\[/quote\\](\r\n|\r|\n)?@si",
        "@\\[code\\](\r\n|\r|\n)*(.*?)(\r\n|\r|\n)*\\[/code\\](\r\n|\r|\n)?@si",
			
        // [center][/center], [right][/right], [justify][/justify],
        // [centerblock][/centerblock] (centers a left-aligned block of text)
        "@\\[center\\](\r\n|\r|\n)?(.*?)(\r\n|\r|\n)?\\[/center\\](\r\n|\r|\n)?@si",
        "@\\[right\\](\r\n|\r|\n)?(.*?)(\r\n|\r|\n)?\\[/right\\](\r\n|\r|\n)?@si",
        "@\\[justify\\](\r\n|\r|\n)?(.*?)(\r\n|\r|\n)?\\[/justify\\](\r\n|\r|\n)?@si",
        "@\\[centerblock\\](\r\n|\r|\n)?(.*?)(\r\n|\r|\n)?\\[/centerblock\\](\r\n|\r|\n)?@si",
			
        // [list][*][/list], [list=][*][/list]
        "@\\[list\\](\r\n|\r|\n)*(.*?)(\r\n|\r|\n)*\\[/list\\](\r\n|\r|\n)?@si",
        "@\\[list=1\\](\r\n|\r|\n)*(.*?)(\r\n|\r|\n)*\\[/list\\](\r\n|\r|\n)?@si",
        "@\\[list=a\\](\r\n|\r|\n)*(.*?)(\r\n|\r|\n)*\\[/list\\](\r\n|\r|\n)?@si",
        "@\\[list=A\\](\r\n|\r|\n)*(.*?)(\r\n|\r|\n)*\\[/list\\](\r\n|\r|\n)?@si",
        "@\\[list=i\\](\r\n|\r|\n)*(.*?)(\r\n|\r|\n)*\\[/list\\](\r\n|\r|\n)?@si",
        "@\\[list=I\\](\r\n|\r|\n)*(.*?)(\r\n|\r|\n)*\\[/list\\](\r\n|\r|\n)?@si",
        //			"@(\r\n|\r|\n)?\\[\\*\\](\r\n|\r|\n)?([^\\[]*)@si",
			
        // [url=][/url], [url][/url], [email][/email]
        "@\\[url=([^\\]\r\n]+)\\](.*?)\\[/url\\]@si",
        "@\\[url\\](.*?)\\[/url\\]@si",
        "@\\[urls=([^\\]\r\n]+)\\](.*?)\\[/urls\\]@si",
        "@\\[urls\\](.*?)\\[/urls\\]@si",
        "@\\[email\\](.*?)\\[/email\\]@si",
        "@\\[a=([^\\]\r\n]+)\\]@si",

        // [youtube]c6zjwPiOHtg[/youtube]
        "@\\[youtube\\](.*?)\\[/youtube\\]@si",

        
        // using soundcloud's wordpress code
        // [soundcloud url="http://api.soundcloud.com/tracks/15220750" ...]
        "@\\[soundcloud\s+url=&quot;http://api\.soundcloud\.com/tracks/(\d+)&quot;[^\]]*\\]@si",

        
        // [img][/img], [img=][/img], [clear]
        "@\\[img\\](.*?)\\[/img\\](\r\n|\r|\n)?@si",
        "@\\[imgl\\](.*?)\\[/imgl\\](\r\n|\r|\n)?@si",
        "@\\[imgr\\](.*?)\\[/imgr\\](\r\n|\r|\n)?@si",
        "@\\[img=([^\\]\r\n]+)\\](.*?)\\[/img\\](\r\n|\r|\n)?@si",
        "@\\[imgl=([^\\]\r\n]+)\\](.*?)\\[/imgl\\](\r\n|\r|\n)?@si",
        "@\\[imgr=([^\\]\r\n]+)\\](.*?)\\[/imgr\\](\r\n|\r|\n)?@si",
        "@\\[clear\\](\r\n|\r|\n)?@si",
			
        // [hr], \n
        "@\\[hr\\](\r\n|\r|\n)?@si",
        "@(\r\n|\r|\n)@");
		
    $replace  = array(
        '<li>$3</li>',
			
		// [b][/b], [i][/i], [u][/u], [mono][/mono]
        '<b>$1</b>',
        '<i>$1</i>',
        '<span style="text-decoration:underline">$1</span>',
        '<span class="mono">$1</span>',
		
        // [color=][/color], [size=][/size]
        '<span style="color:$1">$2</span>',
        '<span style="font-size:$1px">$2</span>',

        // [quote][/quote], [code][/code]
        '<div class="quote"><span style="font-size:0.9em;font-style:italic">$1 wrote:<br /><br /></span>$3</div>',
        '<div class="quote">$2</div>',
        '<div class="code">$2</div>',
			
        // [center][/center], [right][/right], [justify][/justify],
        // [centerblock][/centerblock]
        '<div style="text-align:center">$2</div>',
        '<div style="text-align:right">$2</div>',
        '<div style="text-align:justify">$2</div>',
        '<CENTER><TABLE BORDER=0><TR><TD>$2</TD></TR></TABLE></CENTER>',
			
        // [list][*][/list], [list=][*][/list]
        '<ul>$2</ul>',
        '<ol style="list-style-type:decimal">$2</ol>',
        '<ol style="list-style-type:lower-alpha">$2</ol>',
        '<ol style="list-style-type:upper-alpha">$2</ol>',
        '<ol style="list-style-type:lower-roman">$2</ol>',
        '<ol style="list-style-type:upper-roman">$2</ol>',
        //			'<li />',
			
        // [url=][/url], [url][/url], [email][/email]
        '<a href="$1" rel="external">$2</a>',
        '<a href="$1" rel="external">$1</a>',
        '<a href="$1">$2</a>',
        '<a href="$1">$1</a>',
        '<a href="mailto:$1">$1</a>',
        '<a name="$1"></a>',

        // [youtube]c6zjwPiOHtg[/youtube]
        '<iframe width="560" height="349" '.
        'src="http://www.youtube.com/embed/$1" '.
        'frameborder="0" allowfullscreen></iframe>',

        // [soundcloud url="http://api.soundcloud.com/tracks/15220750" ...]
        // where $1 is the track code, extracted above
        '<object height="81" width="100%"> <param name="movie" value="http://player.soundcloud.com/player.swf?url=http%3A%2F%2Fapi.soundcloud.com%2Ftracks%2F$1&amp;show_comments=false&amp;auto_play=false&amp;color=ff7700"></param> <param name="allowscriptaccess" value="always"></param> <embed allowscriptaccess="always" height="81" src="http://player.soundcloud.com/player.swf?url=http%3A%2F%2Fapi.soundcloud.com%2Ftracks%2F$1&amp;show_comments=false&amp;auto_play=false&amp;color=ff7700" type="application/x-shockwave-flash" width="100%"></embed> </object>',

        
        // [img][/img], [img=][/img], [clear]
        '<center><img style="max-width: 80%; max-height: 640px;" border=0 src="$1" alt="$1" /></center>',
        '<img style="max-width: 80%; max-height: 640px;" border=0 align="left" src="$1" alt="$1" />',
        '<img style="max-width: 80%; max-height: 640px;" border=0 align="right" src="$1" alt="$1" />',
        '<img style="max-width: 80%; max-height: 640px;" border=0 src="$1" alt="$2" title="$2" />',
        '<img style="max-width: 80%; max-height: 640px;" border=0 align="left" src="$1" alt="$2" title="$2" />',
        '<img style="max-width: 80%; max-height: 640px;" border=0 align="right" src="$1" alt="$2" title="$2" />',
        '<div style="clear:both"></div>',
			
        // [hr], \n
        '<hr />',
        '<br />');
    return preg_replace($patterns, $replace, $inData );
    }


?>