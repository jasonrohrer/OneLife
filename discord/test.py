import requests
import json
import time
from datetime import datetime

botToken = "PUT_TOKEN_HERE"


headers = {
    "Accept": "*/*",
    "Content-Type": "application/json",
    "Authorization": "Bot " + botToken
}





# endpoint for posting a message to a channel
endpoint = 'https://discordapp.com/api/channels/1064680416524632134/messages'

postData = '{"content":"Posting as a bot"}';



# endpoint for getting list of members for a server (called a guild in the API)
# this isn't that useful, because members are in ID order
# You can get 1000 at a time, and page through, but there's no way
# to get most recent members
endpoint = 'https://discordapp.com/api/guilds/328215300279500800/members?limit=1000'



# endpoint for generating new invites to a channel
#endpoint = 'https://discordapp.com/api/channels/1064651268229959714/invites'

# invite last forever, can only be used by one person, and are never recycled
#postData = '{ "max_age":0, "max_uses":1, "unique":true }'



# must use PUT command for this:
# endpoint for adding a role to a user
# test user ID:  478252140348047360
# test role ID:  1110719232804655115

#endpoint = 'https://discordapp.com/api/guilds/328215300279500800/members/478252140348047360/roles/1110719232804655115'




# endpoint for creating a new slash command on a specific guild
# uses application ID and guild ID
#endpoint = "https://discord.com/api/v10/applications/991118685697749032/guilds/328215300279500800/commands"

# endpoint for creating a new global slash command (which can be used in DM
# with a bot)
# uses application ID
#endpoint = "https://discord.com/api/v10/applications/991118685697749032/commands"


postData = '{ "name": "unlock", "description":"Unlock the Insiders area.", "type": 1, "options": [ { "name": "secret-words", "description":"Your secret words", "type":3, "required": true } ] }';


# endpoint for deleting guild commands
# use DELETE method (instead of post or get)
#endpoint = "https://discord.com/api/v10/applications/991118685697749032/guilds/328215300279500800/commands/1064962267067719821"

# endpoint for deleting global commands
# use DELETE method (instead of post or get)
#endpoint = "https://discord.com/api/v10/applications/991118685697749032/commands/1065354293009469522"


# endpoint for getting info about roles
# use GET
# endpoint = "https://discordapp.com/api/guilds/328215300279500800/roles"


# endpoint for getting user info
# use GET
# test user ID:  478252140348047360

#endpoint = 'https://discordapp.com/api/users/478252140348047360'



#response = requests.post( endpoint, headers = headers, data = postData )
endpoint = 'https://discordapp.com/api/guilds/328215300279500800/members?limit=1000'

endpointCurrent = endpoint

numFetched = 1000;

oldEnoughIDs = [];


while numFetched == 1000 :
    print( "Fetching " + endpointCurrent )
    response = requests.get( endpointCurrent, headers = headers )
    decoded = json.loads( response.text );
    
    maxID = 0

    for r in decoded :
        
        # trim off time part
        dateString = r[ "joined_at" ]
        dateParts = dateString.split( "T" )
        
        d = datetime.strptime( dateParts[0], "%Y-%m-%d" )
        t = time.mktime( d.timetuple() )
        
        curTime = time.time();

        id = int( r[ "user" ][ "id" ] )

        # print accounts that are at least two weeks old
        if curTime - t > 3600 * 24 * 14 :
            #print( r[ "user" ][ "username" ] )
            #print( r[ "joined_at" ] )
            oldEnoughIDs.append( id )
        
        if id > maxID :
            maxID = id
    numFetched = len( decoded )
    endpointCurrent = endpoint + "&after=" + str( maxID )
    

numOldEnough = len( oldEnoughIDs )

print( "Old enough " + str( numOldEnough ) )


# now give them the OHOL Voting role

# note:  bot must have Manage Roles permissions 
# AND the bot's auto-generated role (when added to the server) must be ABOVE
# the role that it is trying to manage.

countSuccess = 0

for u in oldEnoughIDs :
    time.sleep( 1 )

    endpoint = "https://discordapp.com/api/guilds/328215300279500800/members/" + str( u ) + "/roles/1110719232804655115"
    print( "Fetching endpoint " + endpoint )

    response = requests.put( endpoint, headers = headers )
    
    if response.status_code == 204 :
        countSuccess = countSuccess + 1
    else :
        print( "Failed with code " + str( response.status_code ) )
        print( "Raw response:  " + 
               str( response.content ) )

print( "Added role correctly " + str( countSuccess ) + " times" )
    
#response = requests.delete( endpoint, headers = headers )

#response = requests.put( endpoint, headers = headers )


#print( "Response code:  " + 
#       str( response.status_code ) )

#print( "Raw response:  " + 
#       str( response.content ) )


