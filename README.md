# One Hour One Life+ [![Github All Releases](https://img.shields.io/github/downloads/Awbz/OneLife/total.svg?style=plastic)](https://github.com/Awbz/OneLife/releases/latest) [![GitHub release](https://img.shields.io/github/release/Awbz/OneLife.svg?style=plastic)](https://github.com/Awbz/OneLife/releases/latest) [![Github Releases](https://img.shields.io/github/downloads/Awbz/OneLife/latest/total.svg?style=plastic)](https://github.com/Awbz/OneLife/releases/latest) [![GitHub Release Date](https://img.shields.io/github/release-date/Awbz/OneLife.svg?style=plastic)](https://github.com/Awbz/OneLife/releases/latest)

A lightly modified client for [One Hour One Life](http://onehouronelife.com), a cross-platform and open-source multiplayer survival game of parenting and civilization building by [Jason Rohrer](http://hcsoftware.sf.net/jason-rohrer).

<p align="center">
    <table>
        <tr>
            <td width="16%" align="center"><a href="https://github.com/Awbz/OneLife#requirements">Requirements</a></td>
            <td width="16%" align="center"><a href="https://github.com/Awbz/OneLife#installation">Installation</a></td>
            <td width="16%" align="center"><a href="https://github.com/Awbz/OneLife#usage">Usage</a></td>
            <td width="16%" align="center"><a href="https://github.com/Awbz/OneLife#added_features">Added Features</a></td>
            <td width="16%" align="center"><a href="https://github.com/Awbz/OneLife#credits">Credits</a></td>
            <td width="16%" align="center"><a href="https://github.com/Awbz/OneLife#license">License</a></td>
        </tr>
    </table>
</p>

<p align="center">
    <img align="left" width="15%" src="http://onehouronelife.com/portraitLeft.jpg">
    <img width="50%" src="http://onehouronelife.com/lifeLine.jpg">
    <img width="15%" align="right" src="http://onehouronelife.com/portraitRight.jpg">
    <br />
    <img width="65%" src="http://onehouronelife.com/logo558x305_border.jpg">
</p>

# Requirements
   * Requires the base game to already be installed.
      - Grab it from the download link provided when you [created your official lifetime account](https://sites.fastspring.com/jasonrohrer/instant/onehouronelife?referrer=), or from [Steam](https://store.steampowered.com/app/595690/One_Hour_One_Life/).
   * The full client, modded client, editor, and server are currently only provided for Windows in the "win_full" package.
      - **NOTE**: The "win_full" package does **not** automatically grant you access to play on the official servers!  However, you can use this package to host your own private server(s) directly on Windows.
      - Use **runServer.bat** in the server folder to create the necessary symlinks & launch the server.
   * Pre-compiled releases of just the modified client are available for Windows, Linux and macOS 10.5+.

# Installation
Download the [latest release](https://github.com/Awbz/OneLife/releases/latest) for your OS and extract all 5 files from the zip file into the folder where your game is installed.  There should be 1 executable and 4 text documents.  That's it!

Now just launch the game with OneLife+.exe for Windows, OneLife_v###+.app on macOS (where ### is the client version), or OneLife+ on Linux.


Steam Mod Installation Guide by [KrissJin](https://www.twitch.tv/krissjin):
<p align="center">
   <a href="https://youtu.be/H1u8lD0F0-I">
      <img src="https://img.youtube.com/vi/H1u8lD0F0-I/0.jpg" title="Steam Mod Installation Guide by KrissJin" alt="Steam Mod Installation Guide by KrissJin" width="65%" />
   </a>
</p>

# Usage
Keybind | Description
------- | -----------
**Left ( &larr; )** | Decreases the FOV zoom scale by 0.5x.  Minimum of 1.0x scale.
**Right ( &rarr; )** | Increases the FOV zoom scale by 0.5x.  Maximum of 6.0x scale.
**Shift + Left ( &larr; )** | Sets the FOV zoom scale to your preferred minimum.  Defaults to 1.5x scale.
**Shift + Right ( &rarr; )** | Sets the FOV zoom scale to your preferred maximum.  Defaults to 3.0x scale.
**Grave Key ( ` )** | Toggles HUD scaling.  HUD scales with zoom level or locks at 1.0x, separate from the game scene.
**Backslash Key ( \\ )** | Toggles displaying of the Lineage Fertility panel.
**Shift + Space** | Generate a random name.
**Shift + a-z** | Generate a random name that begins with the corresponding letter that you press.
**Shift + DEL** | Sudden Infant Death hotkey.  Only works if you are less than 1 year old & held by your parent.
**F1 - F7 ( F-Keys )** | Hotkeys for triggering emotions.  Ordered the same as your emotionWords.ini settings file.
**? (Question Mark)** | Toggles the zoomed-in magnifier box that moves with your mouse cursor.

# Added Features
* **In-Game Adjustable FOV Zoom**

   - Adds four new settings that control the FOV "zoom level" of the game, "fovScale", "fovScaleHUD", "fovPreferredMin", and "fovPreferredMax".
   - "fovScale" controls your "zoom factor" and can be any value from 1.0 to 6.0.
   - "fovScaleHUD" toggles locking your HUD at a 1.0x scale, separate from the game's FOV scale.
   - "fovPreferredMin" and "fovPreferredMax" are presets that can you use to quickly switch between a minimum & maximum FOV scale.
   - **NOTE:** Scales above 2x are likely to have significant amounts of "screen popping" around the edges.  This is due to the amount of data that the server sends to the client at any given time and cannot be avoided.  If this bothers you, then just don't zoom out that far!

<p align="center">
    <img alt="720p - No Zoom" width="20%" src="https://user-images.githubusercontent.com/24634429/45483614-4c254d80-b706-11e8-91be-e14ba6d4e00e.png">
    <img alt="1080p - 1.5x Zoom" width="20%" src="https://user-images.githubusercontent.com/24634429/45483638-62330e00-b706-11e8-82da-8e11a92c2e6b.png">
    <img alt="1440p - 2x Zoom" width="20%" src="https://user-images.githubusercontent.com/24634429/45483650-6c550c80-b706-11e8-9226-51bac40c8acf.png">
    <img alt="4k - 3x Zoom" width="20%" src="https://user-images.githubusercontent.com/24634429/45483667-7971fb80-b706-11e8-9246-720bc9f15203.png">
</p>

* **Zoom-IN Magnifier**

   - Toggle a small zoomed-in magnifier, borrowed from the game's Editor.  Great for seeing tiny objects wherever you move your mouse!

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

* **Adjustable Lifespan**

   - Adds a new setting that can be used to control the overall lifespan for clients & servers, "lifespanMultiplier".
   - **NOTE** Settings must match on the client & server for it to properly work as expected.

* **Name Generator**

   - Automatically generate a name that the server allows, as it uses the same first and last name lists.
   - Auto-populates your chat with "YOU ARE <name>", or "I AM <name>" if you have not named yourself yet.
   - Can generate names pseudo-randomly, or that begin with a specified letter.
   - **NOTE:** You can customize the first and last name lists to only pick from your favorites!  Remember that names not already included on these lists will not be recognized by the server.

* **Easy Chat**
   - Auto-focuses the chat box when you start typing.  Never "lose" your chat by forgetting to press Enter first!
   - Includes keybinds for triggering Emotes or Sudden Infant Death commands.

# Credits
Thanks to [Joriom](https://onehouronelife.com/forums/profile.php?id=607) for the [detailed guides](https://onehouronelife.com/forums/viewtopic.php?id=1438), [Bimble](https://onehouronelife.com/forums/profile.php?id=682) for the [VirtualBox image](https://onehouronelife.com/forums/viewtopic.php?id=498), [Drakulon](https://onehouronelife.com/forums/profile.php?id=1165) for the [original FOV mod concept](https://onehouronelife.com/forums/viewtopic.php?id=1422), and [UncleGus](https://github.com/UncleGus) for the [original lifespan concept](https://github.com/UncleGus/OneLife/tree/lifespan).

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
