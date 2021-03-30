# h-encore²

*h-encore²*, where *h* stands for hacks and homebrews, is the fourth public jailbreak for the *PS Vita™* which supports the newest firmwares 3.65-3.73. It allows you to make kernel- and user-modifications, change the clock speed, install plugins, run homebrews and much more.

This mod allows you to switch between 2 config.txt files for different sets of plugins - Portable and Dock. You can also backup your current config.txt and reset to the base config plus the SD2VITA plugin. The switch must happen when first loading CFW, a switch when already on CFW still needs a device reboot. This only works on the ur0 partition so make sure there's no config.txt in the ux0. Please install the original app first to then patch the mod:

https://github.com/TheOfficialFloW/h-encore-2

## Requirements

- Your device must be on firmware 3.65-3.73. If you're on a lower firmware, please decide carefully to what firmware you want to update, then search for a trustable guide on [/r/vitahacks](https://www.reddit.com/r/vitahacks/).
- If your device is a phat OLED model, you need a Memory Card in order to install. There's no need for a Memory Card on Slim/PS TV models, since they already provide an Internal Storage. Make sure you have got at least `270 MB` of free space.
- Your device must be linked to any PSN account (it doesn't need to be activated though). If it is not, then you must restore default settings in order to sign in.

## Mod
## Installation

1. Download the system.dat release - https://github.com/Codiak101/h-encore-2/releases/latest

2. Transfer the file to your Vita (user/??/savedata on your active partition) with VitaShell (can be downloaded from h-encore-2)

3. Open PCSG90096 and backup the existing system.dat file then navigate back to savedata and copy the new system.dat

4. Press Triangle on PCSG90096 and select 'Open decrypted'

5. Press Triangle and select Paste to replace system.dat

6. If using SD2VITA copy the PCSG90096 savedata from ux0 to uma0

## Instructions
![image](https://user-images.githubusercontent.com/78706679/113041457-e6005900-9191-11eb-8278-8e76cf2e3971.png)

When first launching you'll see [NO MODE] where selecting Continue will use your current config.txt as normal and not set a mode. When switching mode your current config.txt will be taken as the Portable config and a new file called config_dock.txt is created. This is the Dock config with the base settings which you can now put your plugins in. When launching again you'll see [PORTABLE MODE] and switching mode will make your Dock config the current config and save your Portable config to config_port.txt. Relaunching shows [DOCK MODE] - you can now switch between the 2 modes.

Menu > selecting a mode exits the app >> selecting another action refreshes the app:

"> Continue" - Load the current config

"> Switch Mode" - Switch between Portable and Dock mode

"> Recovery Mode" - Backup current config to config_bkup.txt and reset config.txt to the base config plus the SD2VITA plugin

">> HENkaku CFW" - Install HENkaku

">> VitaShell App" - Install VitaShell

">> No Trophy Message" - Personalise savedata which removes the trophy message when launching

## Credits

Thanks to TheOfficialFloW for the original h-encore-2
