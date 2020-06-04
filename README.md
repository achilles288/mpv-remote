![mpv logo](https://raw.githubusercontent.com/mpv-player/mpv.io/master/source/images/mpv-logo-128.png)

# MPV Remote

Remote-controlled cross-platform media player for a TV connected to a PC. Such thing exist in a case like when a person plays a media downloaded in his/her laptop connecting to a TV via an **HDMI** cable. In some cases, the PC is used only for media playing. With this **MPV Remote**, the user doesn't have to go back and forth to the TV whenever he/she wants to change the media.

This media player is made controllable from a client device like a portable laptop or a smart phone via **SSH** connection. This media player is derived from an existing open source media player, **MPV Player** which can be seen at [](https://mpv.io/).

## Usage

Assume the PC has already hosted an **SSH** connection and the client device is connected to it via the following command. The example uses **OpenSSH**.
```bash
ssh User@192.168.100.19
```

After the connection, the client should have a command line access to the server PC and a media should be playable by a command like this.
```bash
mpv-remote Videos/jojo-opening1.mp4
```

While the media is playing on the server, the client can control the play by these commands.
```bash
mpv-remote -p    # Pause and resume
mpv-remote -m    # Seek the previous or next part of the media in seconds
mpv-remote -k    # Kill the process. Quit the media player.
```
