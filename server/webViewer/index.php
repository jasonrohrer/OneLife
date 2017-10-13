<?php


$gitPath = "readOnlyOneLifeData/";
$path = "$gitPath/objects/";

echo "<a href=index.php?update=1>Run Update<a><br><br>";


$update = ts_requestFilter( "update", "/[01]/", "0" );

if( $update ) {
    echo "Running update...<br><br>";
    echo exec( "cd $gitPath; git pull" );
    echo "<br>";
    echo exec( "cd $gitPath/sprites; mogrify -format png *.tga" );
    echo "<br>";
    echo exec( "cd $gitPath/overlays; mogrify -format png *.tga" );
    echo "<br>";
    echo "...Done</br></br>";
    }


echo "<a href=$gitPath/overlays>View Overlays</a><br><br>";


$files = scandir( $path );

foreach( $files as $file ) {
    if( strpos( $file, ".txt" ) !== FALSE &&
        $file != "nextObjectNumber.txt" ) {
        $parts = preg_split( "/\./", $file );

        $id = $parts[0];
        
        
        $fullName = "$path$file";

        $contents = file_get_contents( $fullName );
        $lines = preg_split( "/\n/", $contents );

        $name = $lines[1];
        echo "<a href=viewer.html?$id>$name</a><br>";
        }
    }



function ts_requestFilter( $inRequestVariable, $inRegex, $inDefault = "" ) {
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


?>