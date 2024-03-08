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


// path to update trigger flag file
// our PHP script will put a "1" in this file to trigger an update
// must be writable by user that runs PHP web scripts
$updateTriggerFilePath = "/home/jasonrohrer2/dataUpdateFlag.txt";


// where update script log should be read from, for display to user
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