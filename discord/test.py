import requests;

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
endpoint = 'https://discordapp.com/api/guilds/328215300279500800/members?limit=10'



# endpoint for generating new invites to a channel
#endpoint = 'https://discordapp.com/api/channels/1064651268229959714/invites'

# invite last forever, can only be used by one person, and are never recycled
#postData = '{ "max_age":0, "max_uses":1, "unique":true }'



# must use PUT command for this:
# endpoint for adding a role to a user
# test user ID:  478252140348047360
# test role ID:  1064657025386152058

#endpoint = 'https://discordapp.com/api/guilds/328215300279500800/members/478252140348047360/roles/1064657025386152058'




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

response = requests.get( endpoint, headers = headers )

#response = requests.delete( endpoint, headers = headers )

#response = requests.put( endpoint, headers = headers )


print( "Response code:  " + 
       str( response.status_code ) )

print( "Raw response:  " + 
       str( response.content ) )
