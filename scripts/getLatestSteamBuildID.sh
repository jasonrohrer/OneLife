steamcmd +login "jasonrohrergames" +app_update 595690 +quit > /dev/null 
steamcmd +login "jasonrohrergames" +app_info_print 595690 +quit | grep buildid | sed -r "s/.*\"([0-9]+)\".*/\1/"