; Inno Setup script for VISE-8 (Windows VST3). Built by CI; untested on a real Windows host.
#define AppName "VISE-8"
#define AppVersion "1.0.0"

[Setup]
AppId={{6F0C1B7E-3C9A-4D52-9B7B-5A1D2E8F4C10}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=Anvil Dynamics
DefaultDirName={commoncf64}\VST3\VISE-8.vst3
DisableDirPage=yes
DisableProgramGroupPage=yes
Uninstallable=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\dist
OutputBaseFilename=VISE-8-Windows-Installer
Compression=lzma2
SolidCompression=yes
PrivilegesRequired=admin

[Files]
Source: "..\build\VISE8_artefacts\Release\VST3\VISE-8.vst3\*"; DestDir: "{commoncf64}\VST3\VISE-8.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
