; Inno Setup script for Max_2_hours (Protect & Perf)
;
; Per-user installer: it does NOT require administrator rights.
; With PrivilegesRequired=lowest, {autopf} resolves to a folder the current user can
; write to without elevation ({localappdata}\Programs).
;
; Command-line overrides (used by CI):
;   /DAppVersion=1.0.0
;   /DSourceDir=..\dist\max_2_hours-sdl3-windows
;   /DOutputBaseName=max_2_hours-sdl3-setup-x64
;   /DOutputDir=..\dist
;   /DSdlLabel="SDL 3"

#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef SourceDir
  #define SourceDir "..\dist\max_2_hours-sdl3-windows"
#endif
#ifndef OutputBaseName
  #define OutputBaseName "max_2_hours-sdl3-setup-x64"
#endif
#ifndef OutputDir
  #define OutputDir "..\dist"
#endif
#ifndef SdlLabel
  #define SdlLabel "SDL 3"
#endif

#define AppName "Protect & Perf"
#define AppExeName "max_2_hours.exe"

[Setup]
; Fixed GUID: keeps upgrades in place. The SDL 3 build uses a different AppId than the SDL 1.2 build so both can coexist.
AppId={{6B0F3C6E-2D5A-4E55-9A56-8D1B7C0A1F20}-SDL3
AppName={#AppName} ({#SdlLabel})
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion} ({#SdlLabel})
AppPublisher=FromTheRags
AppPublisherURL=https://github.com/FromTheRags/Max_2_hours
AppSupportURL=https://github.com/FromTheRags/Max_2_hours/issues
DefaultDirName={autopf}\Max_2_hours ({#SdlLabel})
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
; No admin rights, no UAC prompt, no elevation choice
PrivilegesRequired=lowest
UsePreviousAppDir=yes
LicenseFile={#SourceDir}\LICENSE
OutputDir={#OutputDir}
OutputBaseFilename={#OutputBaseName}
SetupIconFile={#SourceDir}\assets\clock.ico
UninstallDisplayIcon={app}\{#AppExeName}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
CloseApplications=yes
RestartApplications=no

[Languages]
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#AppName} ({#SdlLabel})"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"
Name: "{autodesktop}\{#AppName} ({#SdlLabel})"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(AppName, '&', '&&')}}"; WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent
