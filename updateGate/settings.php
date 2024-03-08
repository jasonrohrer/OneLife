<?php

// Basic settings
// You must set these for the server to work


// The URL of to the server.php script.
$fullServerURL = "http://localhost/jcr13/updateGate/server.php";

$entryPointURL = "http://localhost/jcr13/updateGate/";


// for checking ticket_id hashes
$ticketServerURL = "http://localhost/jcr13/ticketServer/server.php";


// The URL of the main, public-face website
$mainSiteURL = "http://localhost/jcr13/";


// location of content leader email file (for privacy, should not be web-
// accessible)
$contentLeaderEmailFilePath = "/home/jasonrohrer2/contentLeaderEmail.txt";



// user that runs web PHP scripts must have write access to this file
$serialNumberFilePath = "/var/www/html/jcr13/updateGate/nextNumber.txt";

// path to update script
$updateScriptPath = "/home/jasonrohrer2/checkout/OneLife/scripts/generateDataOnlyDiffBundleAHAP.sh";


// where update script log should be written
$updateScriptLogPath = "/tmp/updateGateLog.txt";



// End Basic settings



// Customization settings

// Adjust these to change the way the server  works.


// so different installations can avoid common challenges and allow
// replay attacks between installations
// a serial number is appended to this as the challenge
$challengePrefix = "updateGate_";



// header and footers for various pages
$header = "include( \"header.php\" );";
$footer = "include( \"footer.php\" );";


?>