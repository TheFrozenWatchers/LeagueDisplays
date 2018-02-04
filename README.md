# League Displays

## Description

This is a full rewrite of the [League Displays](https://na.leagueoflegends.com/en/featured/league-displays) application, which is a utility that allows the user to set high quality images and animations from League of Legends for their lock screen, wallpaper, and screensaver.

Despite having the same name, this project is completely independent from the League Displays application by Riot Games.

In light of the above: this project isn’t endorsed by Riot Games and doesn’t reflect the views or opinions of Riot Games or anyone officially involved in producing or managing League of Legends.

League of Legends and Riot Games are trademarks or registered trademarks of Riot Games, Inc. League of Legends © Riot Games, Inc.

For further clarification, please see the [Legal Jibber Jabber](https://www.riotgames.com/en/legal). 

## What inspired you to create this project?

<img src="https://i.imgur.com/VbqbvRa.gif" width="350px">

## Compatibility

The project currently supports ***only*** GTK2 on X11/xwayland.

Supported desktop environments:

 * Deepin
 * KDE 5 (plasma)
 * Openbox
 * Gnome
 * Budgie
 * Cinnamon
 * MATE
 * XFCE 4
 * LXDE
 * Unity

Setting the wallpaper is generally well supported and has been extensively tested.

Screensaver support depends on XScreensaver, and it is reasonably tested albeit still experimental.

Setting the wallpaper of the lock screen setting is not supported yet but it is a planned feature for the near future.

Support status (tested with latest packages on Arch):

| Desktop environment | AppIndicator | Agent | App | Screensaver |
|---------------------|----------------------|-------|-----|-------------|
| Deepin       | Optional | Works | Works | Works |
| KDE          | Optional | Works | Crashes with `locale::facet::_S_create_c_locale name not valid` | Works |
| Openbox      | Optional | Works (no icon because there is no status bar / taskbar here) | Works | Works |
| KDE/Openbox  | Optional | Works | Crashes with `locale::facet::_S_create_c_locale name not valid` | Works |
| GNOME classic | ??? | Works, but no icon | Works | Works |
| GNOME        | ??? | Works, but no icon | Works | Works |
| Budgie       | Optional | Works | Works | Works |
| Cinnamon     | Optional | Works | Works | Works |
| Mate         | Optional | Works | Works | Works |
| XFCE 4       | Needed | Works | Works | Works |
| LXDE         | Optional | Works | Works | Works |
| Unity        | Needed | Works | Works | Works |

Every desktop environment releated code can be found in `crossde.h` and `crossde.cc`.

Should you favorite desktop environment be missing from the list, feel free to add support.

Pull requests are welcome as long as you are adding support for a *nix environment.

We do not support legacy operating systems such as NSA/Windows or Apple/OSX and will not do, do not ask about it.

## Building and dependencies

### Required dependencies:

 * gtk2
 * libx11
 * For building:
   * wget
   * curl
   * gcc
   * git
   * clang
   * cmake
   * make
   * zip
 * Recommended:
   * appindicator (On some desktop environments you must have this installed to have tray icon)

If you have all of them installed run `./prepare.sh` and it will download and compile the latest version of [CEF source release](http://opensource.spotify.com/cefbuilds/index.html) and [rapidjson](https://github.com/Tencent/rapidjson).
<br>

### Building
Just run `make -j4`. _(You can - of course - replace `-j4` to the thread count you want to use for building)_

### Packaging

If you are a package maintainer for a GNU/Linux distribution, we love you!

Please, get in touch (via the issue tracker) and lets make sure that this project is widely and **easily** available for users.

At this stage, we have not packaged the software for any distribution.

As soon as that changes, we will put references to the individual packages here for ease of use.

## Licensing

This version of League Displays is free software licensed under GPL3.

    Copyright (C) 2018 - The Frozen Watchers

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

