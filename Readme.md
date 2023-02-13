![One Hour One Life](./documentation/art/logo2_1200.png)

## **Table of Contents**
>1. [Description](https://github.com/sweiberg/OneLife-Readme/edit/master/Readme.md#description)
>2. [Compilation](https://github.com/sweiberg/OneLife-Readme/edit/master/Readme.md#compilation)
>3. [Quick Gameplay](https://github.com/sweiberg/OneLife-Readme/edit/master/Readme.md#quick-gameplay)
>4. [Official Links & Documentation](https://github.com/sweiberg/OneLife-Readme/edit/master/Readme.md#official-links--documentation)

## **Description**
> ### A multiplayer survival game of parenting and civilization building. Get born to another player as your mother. Live an entire life in one hour. Have babies of your own in the form of other players. Leave a legacy for the next generation as you help to rebuild civilization from scratch.
> One Hour One Life was created and developed by [**Jason Rohrer**](https://github.com/jasonrohrer).
\
\
>**Official Website**\
>Visit [**OneHourOneLife.com**](http://onehouronelife.com/) to learn more about the game, view leaderboards, and to receive support.
\
\
>**Available For Purchase**\
>[**OneHourOneLife.com**](http://onehouronelife.com/)\
>[**SteamPowered.com**](https://store.steampowered.com/app/595690/One_Hour_One_Life/)
>
>If you purchase through OneHourOneLife.com you will receive:
>>+ <sub>A Windows copy of One Hour One Life on Steam.</sub>
>>+ <sub>A download for DRM-Free Linux and Windows builds.</sub> 
>>+ <sub>The source code to build the official version of the game.</sub>
>
>\
>**Product Disclosure**\
>It's necessary to purchase the game through one of the associated links to play on the officially hosted servers. \
>However, the source files attributed to One Hour One Life and available through this repository are considered public domain.\
\
>[**Official Copyright Notice**](./no_copyright.txt)\
><sub>*Read this copyright notice to better understand the attributions, modifications, trademarks, and distribution of any projects using source material from One Hour One Life.*</sub>

## **Compilation**
>**Purchased**\
>If you've purchased One Hour One Life then you can find instructions on how to compile the official Linux version from source at\
>[**OneHourOneLife.com**](http://onehouronelife.com/compileNotes.php). \
>\
>**This Repository**\
>To compile the Client, Editor, and Server from this repository you will need to use the appropriate script within the [scripts folder](./scripts).\
>><sub>Currently, crossplatform compilation of the source files is not supported, you will need to use Linux to compile both the Linux and Windows versions of One Hour One Life.</sub>\
>><sub>You can use Windows Subsystem for Linux (Ex. Ubuntu on the Windows Store) to compile through Windows.</sub>\
>><sub>You will still need to install the necessary libraries recommended in the Purchased section above.</sub>\
>><sub>Some patches may be required for a proper Windows build, please view the third party section below to get a better idea of what is required.</sub>
>>
>\
>**Quick Install**\
>Download and run [pullAndBuildTestSystem.sh](scripts/pullAndBuildTestSystem.sh) in a new folder (Ex. testSystem)\
>\
>**Launching the Client**\
><sub>Use a command prompt to run the commands below</sub>
>>cd testSystem/OneLife/gameSource\
>>./OneLife
>>
>**Launching the Server**\
><sub>Use a command prompt to run the commands below</sub>
>>cd testSystem/OneLife/server\
>>./OneLifeServer
>>
>**Launching the Editor**\
><sub>Use a command prompt to run the commands below</sub>
>>cd testSystem/OneLife/gameSource\
>>./EditOneLife
>>
>\
>**Third Party**\
>[**miniOneLifeCompile**](https://github.com/risvh/miniOneLifeCompile) is a suite of convenient scripts made to compile the Client, Editor, and Server for Linux and Windows.\
><sub>This is a community created repository, we caution that it is not officially developed or endorsed by the creator of One Hour One Life, Jason Rohrer.</sub>\
><sub>Credits to [connorhsm](https://github.com/connorhsm), [IraCasper](https://github.com/IraCasper), and [risvh](https://github.com/risvh) for their contributions to the miniOneLifeCompile repository.
  

## **Quick Gameplay**
>+ Joining a new game you will be born into the world as another player's child or if you're lucky you will be born as a female adult, otherwise known as an Eve.
>+ Left mouse click is used to interact with most objects in game, such as picking up items, crafting, and eating.
>+ Right mouse click is used to drop or switch items, remove items from containers, and to use your weapons.
>+ The enter key is used to access the chatbox which lets you socialize with other characters, specify your surname, or name your children.
>>+ <sub>Ex. MY NAME IS DOE</sub>
>>+ <sub>Ex. YOUR NAME IS JANE</sub>
>+ You will not be able to procreate after the age of 40, so ensure not to waste time as the name suggests you have one hour to live your life (Should you not die sooner).
>+ Pay attention to your food meter and ensure you keep it as full as possible, there are many options such as fruits and herbs to eat when starting out.
>+ You will largely be harvesting resources from materials found around you in the world, which will then be used for crafting.
>+ Remember that One Hour One Life is a game of cooperation, work with friendly players to develop a more advanced civilization.

## **Official Links & Documentation**
>[**Official Website**](https://onehouronelife.com)\
>[**Support Forums**](https://onehouronelife.com/forums/)\
>[**Gameplay Controls**](./documentation/Readme.txt)\
>[**Editor and Server Builds**](./documentation/EditorAndServerBuildNotes.txt)\
>[**Editor Notes**](./documentation/EditorNotes.txt)
