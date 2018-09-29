# One Hour One Life+
A lightly modified client for [One Hour One Life](http://onehouronelife.com), a cross-platform and open-source multiplayer survival game of parenting and civilization building by [Jason Rohrer](http://hcsoftware.sf.net/jason-rohrer).

<p align="center">
    <img align="left" width="15%" src="http://onehouronelife.com/portraitLeft.jpg">
    <img width="50%" src="http://onehouronelife.com/lifeLine.jpg">
    <img width="15%" align="right" src="http://onehouronelife.com/portraitRight.jpg">
    <br />
    <img width="65%" src="http://onehouronelife.com/logo558x305_border.jpg">
</p>

# Requirements
   * Requires the base game to already be installed.
      - Grab it from the download link provided when you [created your official lifetime account](https://sites.fastspring.com/jasonrohrer/instant/onehouronelife?referrer=), or [compiled by Joriom](http://ohol.joriom.pl/download/OneLife_win32_latest) from the [official forums](https://onehouronelife.com/forums/viewtopic.php?id=1467).
   * Pre-compiled releases of this modified client are currently for Windows only.  Feel free to compile from source for Mac/Linux.

# Installation
Download the [latest release](https://github.com/Awbz/OneLife/releases/latest) and extract the zip file's contents into the folder where your game is installed.  That's it!  Now just launch the game with OneLife+.exe.

# Usage
Keybind | Description
------- | -----------
**Left Arrow ( &larr; )** | Decreases the FOV zoom scale by 0.5x.  Minimum of 1.0x scale.
**Right Arrow ( &rarr; )** | Increases the FOV zoom scale by 0.5x.  Maximum of 6.0x scale.
**Grave Key ( ` )** | Toggles HUD scaling.  HUD scales with zoom level or locks at 1.0x, separate from the game scene.
**Backslash Key ( \\ )** | Toggles displaying of the Lineage Fertility panel.
**Shift + DEL** | Gives up on life and allows you to instantly respawn.
**Shift + Space** | Generate a random name.
**Shift + a-z** | Generate a random name that begins with the corresponding letter that you press.

# Added Features
* **In-Game Adjustable FOV Zoom**

   - Adds two new settings that control the FOV "zoom level" of the game, "fovScale" and "fovScaleHUD".
   - "fovScale" controls your "zoom factor" and can be any value from 1.0 to 6.0.
   - "fovScaleHUD" toggles locking your HUD at a 1.0x scale, separate from the game's FOV scale.
   - **NOTE:** Scales above 2x are likely to have significant amounts of "screen popping" around the edges.  This is due to the amount of data that the server sends to the client at any given time and cannot be avoided.  If this bothers you, try using a version with lower resolution.

<p align="center">
    <img alt="720p - No Zoom" width="20%" src="https://user-images.githubusercontent.com/24634429/45483614-4c254d80-b706-11e8-91be-e14ba6d4e00e.png">
    <img alt="1080p - 1.5x Zoom" width="20%" src="https://user-images.githubusercontent.com/24634429/45483638-62330e00-b706-11e8-82da-8e11a92c2e6b.png">
    <img alt="1440p - 2x Zoom" width="20%" src="https://user-images.githubusercontent.com/24634429/45483650-6c550c80-b706-11e8-9226-51bac40c8acf.png">
    <img alt="4k - 3x Zoom" width="20%" src="https://user-images.githubusercontent.com/24634429/45483667-7971fb80-b706-11e8-9246-720bc9f15203.png">
</p>

* **Lineage Fertility Panel**

   - Shows the fertility status for you and your mother.  Statuses are:  Incapable (for males), Too Young, Fertile, Too Old, or Dead. 
   - Also displays counts of any fertile living female relatives, not including yourself, separated by children and adults.
   - Helpful for knowing if you or your mother can breed/feed, or determining the probability of survival for your family's lineage.

<p align="center">
    <img alt="Lineage Fertility Panel" src="https://user-images.githubusercontent.com/24634429/45483594-3ca60480-b706-11e8-9515-6dda2ef10c49.png">
</p>

* **Age Display**

   - Adds your current character's age to the HUD's bottom information bar.

<p align="center">
    <img alt="Age Display" src="https://user-images.githubusercontent.com/24634429/45483575-344dc980-b706-11e8-9cd1-194da65c1965.png">
</p>

* **Name Generator**

   - Automatically generate a name that the server allows, as it uses the same first and last name lists.
   - Auto-populates your chat with "YOU ARE <name>", or "I AM <name>" if you have not named yourself yet.
   - Can generate names pseudo-randomly, or that begin with a specified letter.
   - You can customize the first and last name lists to only pick from your favorites!  Remember that names not already included on these lists will not be recognized by the server.

* **Curse Level Display**
   - Attempts to display your current curse level just below your curse token.
   - Hovering over other players attempts to prefix their displayed information with their curse level. 
   - **NOTE:** Curse level, in this context, is either 1 or 0 if a player is cursed or not.  Currently __does not__ show the number of times someone has been cursed.

* **Seppuku Respawn**
   - Are you a player that prefers to respawn until you get to a desired area or family?  Tired of having to relaunch your client to respawn?  This can help!
   - Assigns a hotkey combination to instantly kill yourself, allowing you to quickly respawn.  Give up on life 200% faster!

# Credits
Thanks to [Joriom](https://onehouronelife.com/forums/profile.php?id=607) for the [detailed guides](https://onehouronelife.com/forums/viewtopic.php?id=1438), [Bimble](https://onehouronelife.com/forums/profile.php?id=682) for the [VirtualBox image](https://onehouronelife.com/forums/viewtopic.php?id=498), and [Drakulon](https://onehouronelife.com/forums/profile.php?id=1165) for the [original FOV mod concept](https://onehouronelife.com/forums/viewtopic.php?id=1422).

Special thanks to these awesome people!
* [2LaughOr2Cry](https://www.twitch.tv/2laughor2cry)
* [Caderly](https://www.twitch.tv/caderly)
* [EliceaBluBear](https://www.twitch.tv/eliceabluebear)
* [EnochZebedee](https://www.twitch.tv/enochzebedee)
* [ipoor2005](https://www.twitch.tv/ipoor2005)
* [LostScholar](https://www.twitch.tv/lostscholar)
* [QuirkySmirkyIan](https://www.twitch.tv/quirkysmirkyian)
* [Tana](https://www.twitch.tv/tana_)
* [Twisted_100](https://www.twitch.tv/twisted_100) / [HoneyBunnyGames](https://www.youtube.com/user/HoneyBunnyGames)
* [Wolvenscar](https://www.twitch.tv/wolvenscar)
* [xclame](https://www.twitch.tv/xclame)
* ...and everyone else on Twitch who tested the mod with feedback, encouragement, and inspiration! <3

# License
<p align="center">
    <img width="10%" src="https://camo.githubusercontent.com/0a683ace13fd155db2030a621d4ce652d9455c79/68747470733a2f2f6372656174697665636f6d6d6f6e732e6769746875622e696f2f63632d636572742d636f72652f696d616765732f636f707972696768742f7075626c6963646f6d61696e2e706e67">
</p>

To the extent possible under law, [Jason Rohrer](http://hcsoftware.sf.net/jason-rohrer) has waived all copyright and related or neighboring rights to this work.  Ditto.