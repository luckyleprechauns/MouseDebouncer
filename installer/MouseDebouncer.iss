; MouseDebouncer - Inno Setup Installer Script
; Requires Inno Setup 6.x (https://jrsoftware.org/isinfo.php)

#define MyAppName "Mouse Debouncer"
#define MyAppVersion "2.0"
#define MyAppPublisher "luckyleprechauns"
#define MyAppURL "https://github.com/luckyleprechauns/MouseDebouncer"
#define MyAppExeName "MouseDebouncer.exe"

[Setup]
AppId={{B8F3A2E1-7C4D-4E5F-9A1B-2D3E4F5A6B7C}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}/issues
AppUpdatesURL={#MyAppURL}/releases
DefaultDirName={autopf}\MouseDebouncer
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir=..\build\installer
OutputBaseFilename=MouseDebouncer-{#MyAppVersion}-setup
SetupIconFile=..\resources\mouse.ico
UninstallDisplayIcon={app}\{#MyAppExeName}
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "startupentry"; Description: "Start Mouse Debouncer on Windows login (10 second delay)"; GroupDescription: "Startup:"

[Files]
Source: "..\build\bin\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
; Start Menu shortcut
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
; Desktop shortcut (optional)
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[Registry]
; Create startup entry with delay using Task Scheduler (handled by [Code] section)
; Clean up app registry settings on uninstall
Root: HKCU; Subkey: "Software\MouseDebouncer"; Flags: uninsdeletekey

[Code]
procedure CreateStartupTask();
var
  ResultCode: Integer;
begin
  Exec('schtasks.exe',
    '/create /tn "MouseDebouncer" /tr "\"' + ExpandConstant('{app}\{#MyAppExeName}') + '\"" /sc onlogon /delay 0000:10 /rl limited /f',
    '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
end;

procedure RemoveStartupTask();
var
  ResultCode: Integer;
begin
  Exec('schtasks.exe',
    '/delete /tn "MouseDebouncer" /f',
    '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    if IsTaskSelected('startupentry') then
      CreateStartupTask();
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
  begin
    RemoveStartupTask();
  end;
end;
