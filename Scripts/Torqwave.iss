; The Windows installer, built by Scripts/package-windows.ps1 with Inno Setup 6:
;   ISCC /DAppVersion=<version> /DStage=<folder with Torqwave.exe and the licences>
;        /DIconFile=<the app's .ico> /DOutputDir=<where the installer goes> Scripts\Torqwave.iss
;
; Installs for the current user (no administrator rights), or for everyone when asked to (/ALLUSERS, or
; the choice Setup offers an administrator). It adds a
; Start menu entry, puts Torqwave in Explorer's Open With list for the audio files it opens
; (AudioFileIO::openedFromFinder), and, as a task, opens sessions and projects with it.

#ifndef AppVersion
  #error AppVersion must be given (/DAppVersion=...)
#endif
#ifndef Stage
  #error Stage must be given (/DStage=...)
#endif
#ifndef OutputDir
  #define OutputDir "."
#endif

#define AppExe "Torqwave.exe"

[Setup]
AppId={{82351F7F-F7FC-4F48-B875-D9356B09DE47}
AppName=Torqwave
AppVersion={#AppVersion}
AppVerName=Torqwave {#AppVersion}
AppPublisher=Torqwave
AppPublisherURL=https://github.com/BobClawblaw/torqwave
AppSupportURL=https://github.com/BobClawblaw/torqwave/issues
AppUpdatesURL=https://github.com/BobClawblaw/torqwave/releases
DefaultDirName={autopf}\Torqwave
; The app was called SpectraBlade up to 0.15.10 and keeps its AppId: an upgrade goes in Torqwave's
; folder, not the old one, which [InstallDelete] removes.
UsePreviousAppDir=no
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
LicenseFile={#Stage}\LICENSE.txt
OutputDir={#OutputDir}
OutputBaseFilename=Torqwave-{#AppVersion}-windows-x64-setup
#ifdef IconFile
SetupIconFile={#IconFile}
#endif
UninstallDisplayIcon={app}\{#AppExe}
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ChangesAssociations=yes
CloseApplications=yes

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "associate"; Description: "Open Torqwave sessions, projects, sequences and kits (.tqsession, .tqproject, .tqseq, .tqkit, and SpectraBlade's .sbsession, .sbproject) with Torqwave"; GroupDescription: "File types:"

[InstallDelete]
Type: filesandordirs; Name: "{autopf}\SpectraBlade"

[Files]
Source: "{#Stage}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs

[Icons]
Name: "{autoprograms}\Torqwave"; Filename: "{app}\{#AppExe}"
Name: "{autodesktop}\Torqwave"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Registry]
; The kinds of file Torqwave opens, each opening with it.
Root: HKA; Subkey: "Software\Classes\Torqwave.Audio"; ValueType: string; ValueData: "Audio file"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\Torqwave.Audio\DefaultIcon"; ValueType: string; ValueData: "{app}\{#AppExe},0"
Root: HKA; Subkey: "Software\Classes\Torqwave.Audio\shell\open\command"; ValueType: string; ValueData: """{app}\{#AppExe}"" ""%1"""
Root: HKA; Subkey: "Software\Classes\Torqwave.Session"; ValueType: string; ValueData: "Torqwave Session"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\Torqwave.Session\DefaultIcon"; ValueType: string; ValueData: "{app}\{#AppExe},0"
Root: HKA; Subkey: "Software\Classes\Torqwave.Session\shell\open\command"; ValueType: string; ValueData: """{app}\{#AppExe}"" ""%1"""
Root: HKA; Subkey: "Software\Classes\Torqwave.Project"; ValueType: string; ValueData: "Torqwave Project"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\Torqwave.Project\DefaultIcon"; ValueType: string; ValueData: "{app}\{#AppExe},0"
Root: HKA; Subkey: "Software\Classes\Torqwave.Project\shell\open\command"; ValueType: string; ValueData: """{app}\{#AppExe}"" ""%1"""
Root: HKA; Subkey: "Software\Classes\Torqwave.Sequence"; ValueType: string; ValueData: "Torqwave Sequence"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\Torqwave.Sequence\DefaultIcon"; ValueType: string; ValueData: "{app}\{#AppExe},0"
Root: HKA; Subkey: "Software\Classes\Torqwave.Sequence\shell\open\command"; ValueType: string; ValueData: """{app}\{#AppExe}"" ""%1"""
Root: HKA; Subkey: "Software\Classes\Torqwave.Kit"; ValueType: string; ValueData: "Torqwave Kit"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\Torqwave.Kit\DefaultIcon"; ValueType: string; ValueData: "{app}\{#AppExe},0"
Root: HKA; Subkey: "Software\Classes\Torqwave.Kit\shell\open\command"; ValueType: string; ValueData: """{app}\{#AppExe}"" ""%1"""

; The extensions' keys: taken away on uninstalling only if nothing else is left in them (another
; program's entries keep them). Listed before what goes in them, as uninstalling works last first.
Root: HKA; Subkey: "Software\Classes\.tqsession"; Flags: uninsdeletekeyifempty
Root: HKA; Subkey: "Software\Classes\.tqproject"; Flags: uninsdeletekeyifempty
Root: HKA; Subkey: "Software\Classes\.tqseq"; Flags: uninsdeletekeyifempty
Root: HKA; Subkey: "Software\Classes\.tqkit"; Flags: uninsdeletekeyifempty
Root: HKA; Subkey: "Software\Classes\.sbsession"; Flags: uninsdeletekeyifempty
Root: HKA; Subkey: "Software\Classes\.sbproject"; Flags: uninsdeletekeyifempty
Root: HKA; Subkey: "Software\Classes\.wav"; Flags: uninsdeletekeyifempty
Root: HKA; Subkey: "Software\Classes\.aif"; Flags: uninsdeletekeyifempty
Root: HKA; Subkey: "Software\Classes\.aiff"; Flags: uninsdeletekeyifempty
Root: HKA; Subkey: "Software\Classes\.flac"; Flags: uninsdeletekeyifempty
Root: HKA; Subkey: "Software\Classes\.ogg"; Flags: uninsdeletekeyifempty
Root: HKA; Subkey: "Software\Classes\.mp3"; Flags: uninsdeletekeyifempty
Root: HKA; Subkey: "Software\Classes\.m4a"; Flags: uninsdeletekeyifempty

; Sessions, projects, sequences and kits open with Torqwave, if chosen; it is in their Open With list either way.
Root: HKA; Subkey: "Software\Classes\.tqsession"; ValueType: string; ValueName: ""; ValueData: "Torqwave.Session"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.tqproject"; ValueType: string; ValueName: ""; ValueData: "Torqwave.Project"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.tqsession\OpenWithProgids"; ValueType: string; ValueName: "Torqwave.Session"; ValueData: ""; Flags: uninsdeletevalue uninsdeletekeyifempty
Root: HKA; Subkey: "Software\Classes\.tqproject\OpenWithProgids"; ValueType: string; ValueName: "Torqwave.Project"; ValueData: ""; Flags: uninsdeletevalue uninsdeletekeyifempty
Root: HKA; Subkey: "Software\Classes\.tqseq"; ValueType: string; ValueName: ""; ValueData: "Torqwave.Sequence"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.tqkit"; ValueType: string; ValueName: ""; ValueData: "Torqwave.Kit"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.tqseq\OpenWithProgids"; ValueType: string; ValueName: "Torqwave.Sequence"; ValueData: ""; Flags: uninsdeletevalue uninsdeletekeyifempty
Root: HKA; Subkey: "Software\Classes\.tqkit\OpenWithProgids"; ValueType: string; ValueName: "Torqwave.Kit"; ValueData: ""; Flags: uninsdeletevalue uninsdeletekeyifempty

; SpectraBlade's sessions and projects open with Torqwave too.
Root: HKA; Subkey: "Software\Classes\.sbsession"; ValueType: string; ValueName: ""; ValueData: "Torqwave.Session"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.sbproject"; ValueType: string; ValueName: ""; ValueData: "Torqwave.Project"; Flags: uninsdeletevalue; Tasks: associate
Root: HKA; Subkey: "Software\Classes\.sbsession\OpenWithProgids"; ValueType: string; ValueName: "Torqwave.Session"; ValueData: ""; Flags: uninsdeletevalue uninsdeletekeyifempty
Root: HKA; Subkey: "Software\Classes\.sbproject\OpenWithProgids"; ValueType: string; ValueName: "Torqwave.Project"; ValueData: ""; Flags: uninsdeletevalue uninsdeletekeyifempty

; What SpectraBlade registered, taken away on installing.
Root: HKA; Subkey: "Software\Classes\SpectraBlade.Session"; Flags: deletekey
Root: HKA; Subkey: "Software\Classes\SpectraBlade.Project"; Flags: deletekey
Root: HKA; Subkey: "Software\Classes\SpectraBlade.Audio"; Flags: deletekey
Root: HKA; Subkey: "Software\Classes\Applications\SpectraBlade.exe"; Flags: deletekey
Root: HKA; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\SpectraBlade.exe"; Flags: deletekey

; Audio files keep whatever opens them now: Torqwave is offered in their Open With list.
Root: HKA; Subkey: "Software\Classes\.wav\OpenWithProgids"; ValueType: string; ValueName: "Torqwave.Audio"; ValueData: ""; Flags: uninsdeletevalue uninsdeletekeyifempty
Root: HKA; Subkey: "Software\Classes\.aif\OpenWithProgids"; ValueType: string; ValueName: "Torqwave.Audio"; ValueData: ""; Flags: uninsdeletevalue uninsdeletekeyifempty
Root: HKA; Subkey: "Software\Classes\.aiff\OpenWithProgids"; ValueType: string; ValueName: "Torqwave.Audio"; ValueData: ""; Flags: uninsdeletevalue uninsdeletekeyifempty
Root: HKA; Subkey: "Software\Classes\.flac\OpenWithProgids"; ValueType: string; ValueName: "Torqwave.Audio"; ValueData: ""; Flags: uninsdeletevalue uninsdeletekeyifempty
Root: HKA; Subkey: "Software\Classes\.ogg\OpenWithProgids"; ValueType: string; ValueName: "Torqwave.Audio"; ValueData: ""; Flags: uninsdeletevalue uninsdeletekeyifempty
Root: HKA; Subkey: "Software\Classes\.mp3\OpenWithProgids"; ValueType: string; ValueName: "Torqwave.Audio"; ValueData: ""; Flags: uninsdeletevalue uninsdeletekeyifempty
Root: HKA; Subkey: "Software\Classes\.m4a\OpenWithProgids"; ValueType: string; ValueName: "Torqwave.Audio"; ValueData: ""; Flags: uninsdeletevalue uninsdeletekeyifempty

; The program itself: its name in Open With, the types it takes, and "Torqwave" in Run (Win+R).
Root: HKA; Subkey: "Software\Classes\Applications\{#AppExe}"; ValueType: string; ValueName: "FriendlyAppName"; ValueData: "Torqwave"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\Applications\{#AppExe}\shell\open\command"; ValueType: string; ValueData: """{app}\{#AppExe}"" ""%1"""
Root: HKA; Subkey: "Software\Classes\Applications\{#AppExe}\SupportedTypes"; ValueType: string; ValueName: ".wav"; ValueData: ""
Root: HKA; Subkey: "Software\Classes\Applications\{#AppExe}\SupportedTypes"; ValueType: string; ValueName: ".aif"; ValueData: ""
Root: HKA; Subkey: "Software\Classes\Applications\{#AppExe}\SupportedTypes"; ValueType: string; ValueName: ".aiff"; ValueData: ""
Root: HKA; Subkey: "Software\Classes\Applications\{#AppExe}\SupportedTypes"; ValueType: string; ValueName: ".flac"; ValueData: ""
Root: HKA; Subkey: "Software\Classes\Applications\{#AppExe}\SupportedTypes"; ValueType: string; ValueName: ".ogg"; ValueData: ""
Root: HKA; Subkey: "Software\Classes\Applications\{#AppExe}\SupportedTypes"; ValueType: string; ValueName: ".mp3"; ValueData: ""
Root: HKA; Subkey: "Software\Classes\Applications\{#AppExe}\SupportedTypes"; ValueType: string; ValueName: ".m4a"; ValueData: ""
Root: HKA; Subkey: "Software\Classes\Applications\{#AppExe}\SupportedTypes"; ValueType: string; ValueName: ".tqsession"; ValueData: ""
Root: HKA; Subkey: "Software\Classes\Applications\{#AppExe}\SupportedTypes"; ValueType: string; ValueName: ".tqproject"; ValueData: ""
Root: HKA; Subkey: "Software\Classes\Applications\{#AppExe}\SupportedTypes"; ValueType: string; ValueName: ".sbsession"; ValueData: ""
Root: HKA; Subkey: "Software\Classes\Applications\{#AppExe}\SupportedTypes"; ValueType: string; ValueName: ".sbproject"; ValueData: ""
Root: HKA; Subkey: "Software\Classes\Applications\{#AppExe}\SupportedTypes"; ValueType: string; ValueName: ".tqseq"; ValueData: ""
Root: HKA; Subkey: "Software\Classes\Applications\{#AppExe}\SupportedTypes"; ValueType: string; ValueName: ".tqkit"; ValueData: ""
Root: HKA; Subkey: "Software\Microsoft\Windows\CurrentVersion\App Paths\{#AppExe}"; ValueType: string; ValueData: "{app}\{#AppExe}"; Flags: uninsdeletekey

[Run]
Filename: "{app}\{#AppExe}"; Description: "{cm:LaunchProgram,Torqwave}"; Flags: nowait postinstall skipifsilent
