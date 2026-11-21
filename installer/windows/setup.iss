; Double-click Windows installer for a Rave Academy VST3 plug-in (Inno Setup 6).
; iscc /DAppName="Rave Academy Instant Pump" /DAppId=instantpump /DAppVersion=1.0.0
;      /DSourceVst3="path\to\Rave Academy Instant Pump.vst3" /DOutName=... setup.iss
#ifndef AppName
  #error Pass /DAppName
#endif

[Setup]
AppId={{RaveAcademy-{#AppId}}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=Rave Academy
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
Uninstallable=yes
UninstallDisplayName={#AppName}
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=.
OutputBaseFilename={#OutName}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "polish"; MessagesFile: "compiler:Languages\Polish.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "{#SourceVst3}\*"; DestDir: "{commoncf64}\VST3\{#AppName}.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\{#AppName}.vst3"

[Messages]
polish.FinishedLabel=Wtyczka {#AppName} jest zainstalowana.%n%nUruchom Abletona (jeśli był otwarty: Settings - Plug-Ins - Rescan).%nWtyczka jest w przeglądarce: Plug-Ins - Rave Academy.
english.FinishedLabel={#AppName} is installed.%n%nStart Ableton Live (if it was open: Settings - Plug-Ins - Rescan).%nFind it under Plug-Ins - Rave Academy.
