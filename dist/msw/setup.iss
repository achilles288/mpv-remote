; -- MPV-Remote.iss --
;
; Remote-controlled cross-platform media player for a TV connected to a PC.
; Such thing exist in a case like when a person plays a media downloaded in
; his/her laptop connecting to a TV via an HDMI cable. In some cases, the PC
; is used only for media playing. With this MPV Remote, the user doesn't have
; to go back and forth to the TV whenever he/she wants to change the media.

[Setup]
AppName=MPV Remote
AppVersion=1.1.0                 
AppPublisher=Khant Kyaw Khaung
LicenseFile=..\..\LICENSE
DefaultDirName={autopf}\MPV Remote          
ChangesAssociations=yes
DisableProgramGroupPage=yes
PrivilegesRequiredOverridesAllowed=dialog
UsePreviousPrivileges=no
Compression=lzma2
SolidCompression=yes
OutputDir=userdocs:MVP Remote Setup
ArchitecturesAllowed=x64           
ArchitecturesInstallIn64BitMode=x64       
WizardStyle=modern

#define BuildDirectory "..\..\build\bin"

[Files]
Source: "..\..\http\*"; DestDir: "{app}\http"; Flags: ignoreversion recursesubdirs
Source: "..\..\external\bin\*"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDirectory}\mpv-play.exe"; DestDir: "{app}"  
Source: "{#BuildDirectory}\mpv-remote.exe"; DestDir: "{app}" 
Source: "{#BuildDirectory}\mpv-remote.dll"; DestDir: "{app}"              
Source: "mpv-play-start.bat"; DestDir: "{app}"                              
Source: "mpv-play.bat"; DestDir: "{app}\cmd"                           
Source: "mpv-remote.bat"; DestDir: "{app}\cmd"                                            

[Run]
Filename: "{app}\mpv-play-start.bat"; Description: "{cm:LaunchProgram,MPV remote player}"; Flags: nowait postinstall skipifsilent

[Tasks]
Name: "StartMenuEntry" ; Description: "Start MPV remote player when Windows starts" ; GroupDescription: "Windows Startup"; MinVersion: 4,4;