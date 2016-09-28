var canvas = document.createElement("canvas");
var ctx = canvas.getContext("2d");
canvas.width = 256;
canvas.height = 512;
document.body.appendChild(canvas);

function rotateAndPaintImage( context, image, angleInRad, 
                              positionX, positionY, axisX, axisY,
                              flipH ) {
    context.translate( positionX, positionY );

    context.rotate( angleInRad );

    if( flipH == 1) {
        context.scale( -1, 1 );
        }
    context.drawImage( image, -axisX, -axisY );
    if( flipH == 1) {
        context.scale( -1, 1 );
        }

    context.rotate( -angleInRad );

    context.translate( -positionX, -positionY );
    }


var TO_RADIANS = Math.PI * 2; 




var loadedImagesCount = 0;
var imageNames = [];
var imagesArray = [];

var clientArray = [];

var drawingCalled = 0;

function doDrawing() {
    ctx.translate( 128, 256 );
    var index = 0;
    imagesArray.forEach(
        function(entry) {
            if( entry.s_ageRange[0] < 20 &&
                ( entry.s_ageRange[1] > 20 || entry.s_ageRange[1] == -1 ) ) {
                //alert( entry.s_rot );
                rotateAndPaintImage( ctx, entry, 
                                     parseFloat( entry.s_rot ) * TO_RADIANS, 
                                     parseFloat( entry.s_offset[0] ),
                                     -parseFloat( entry.s_offset[1] ), 
                                     entry.width/2, entry.height/2,
                                     entry.s_hflip );
                }
            index++;
            }
        );
    ctx.translate( -128, -256 );
    }




var client = new XMLHttpRequest();


function processObjectFile() {
    var lines = client.responseText.split("\n");
    var numSprites = -1;
    var currentLine = 0;

    var id = lines[currentLine].split("=")[1];    
    currentLine++;
    
    var name = lines[currentLine];
    currentLine++;

    var descriptionSpan = document.createElement( "span" );
    descriptionSpan.innerHTML = 
        "[<a href=index.php>Return to List</a>]<br>"
        + "Object "+ id + ":  " + name;

    document.body.insertBefore( descriptionSpan, canvas );
    document.body.insertBefore( document.createElement( "br" ), canvas );
    
    while( lines[currentLine].indexOf( "numSprites" ) == -1 ) {
        currentLine++;
        }
    numSprites = lines[currentLine].split("=")[1];
    currentLine++;

    for( i=0; i<numSprites; i++ ) {
        (function() {  // start closure
        var image = new Image();

        image.s_id = lines[currentLine].split("=")[1];
        currentLine++;
        
        image.s_offset = lines[currentLine].split("=")[1].split(",");
        currentLine++;

        image.s_rot = lines[currentLine].split("=")[1];
        currentLine++;

        image.s_hflip = lines[currentLine].split("=")[1];
        currentLine++;

        image.s_color = lines[currentLine].split("=")[1].split(",");
        currentLine++;
        
        image.s_ageRange = lines[currentLine].split("=")[1].split(",");
        currentLine++;

        // skip parent and invis holding
        currentLine+=2;

        var path = "";

            
        var spriteInfoClient = new XMLHttpRequest();
        
        var infoPath = "readOnlyOneLifeData/sprites/" + image.s_id + ".txt";
        //alert( infoPath );
        //      spritInfoClient.myPath = infoPath;
        
        spriteInfoClient.open( 'GET', infoPath );
        
        
        spriteInfoClient.onreadystatechange = function() {
            //alert( spriteInfoClient.statusText );
            
            if( spriteInfoClient.readyState === XMLHttpRequest.DONE 
                && spriteInfoClient.status === 200 ) {

                //alert( spriteInfoClient.responseText );
                
                var infoParts = spriteInfoClient.responseText.split(" ");

                var xOff = infoParts[2];
                var yOff = infoParts[3];
                //alert( infoParts[0] + " "  + infoParts[1] + " offset = " +
                //       xOff + " " + yOff );

                image.s_offset[0] = parseFloat( image.s_offset[0] ) -
                    parseFloat( xOff );
                image.s_offset[1] = parseFloat( image.s_offset[1] ) +
                    parseFloat( yOff );
                
                
                image.src = path.concat( "readOnlyOneLifeData/sprites/",
                                         image.s_id, ".png" );
	
                image.onload = function(){
                    loadedImagesCount++;
                    if( loadedImagesCount >= numSprites &&
                        drawingCalled == 0 ) {
                        //loaded all pictures
                        drawingCalled = 1;
                        doDrawing();
                        }
                    }
                }
            }
        
        imagesArray.push(image);
        clientArray.push( spriteInfoClient );
        spriteInfoClient.send();
        
        /*
          spriteID=6
          pos=15.000000,5.000000
          rot=0.000000
          hFlip=0
          color=0.985915,0.776059,0.472129
          ageRange=-1.000000,-1.000000
          parent=-1
          invisHolding=0
         */
            }
         )(); // end closure
        
        }
    }


function loadObjectID( id ) {
    
    client.open('GET', 'readOnlyOneLifeData/objects/' + id + '.txt');
    client.onreadystatechange = function() {
        if( client.readyState === XMLHttpRequest.DONE 
            && client.status === 200 ) {
            
            processObjectFile();
            }
        }
    client.send();
    }


loadObjectID( document.location.search.split( "?" )[1] );



