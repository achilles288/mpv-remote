; -- MPV-Remote.iss --
;
; Remote-controlled cross-platform media player for a TV connected to a PC.
; Such thing exist in a case like when a person plays a media downloaded in
; his/her laptop connecting to a TV via an HDMI cable. In some cases, the PC
; is used only for media playing. With this MPV Remote, the user doesn't have
; to go back and forth to the TV whenever he/she wants to change the media.

[Setup]
AppName=MPV Remote
AppVersion=1.0.1
WizardStyle=modern
DefaultDirName={autopf}\MPV Remote
DefaultGroupName=MPV
Compression=lzma2
SolidCompression=yes
OutputDir=userdocs:MVP Remote Setup
ArchitecturesAllowed=x64           
ArchitecturesInstallIn64BitMode=x64

[Files]                                   
Source: "d3dcompiler_43.dll"; DestDir: "{app}" 
Source: "mpv-1.dll"; DestDir: "{app}"
Source: "mpv-play.exe"; DestDir: "{app}"
Source: "mpv-remote.exe"; DestDir: "{app}"
Source: "README.md"; DestDir: "{app}"; Flags: isreadme