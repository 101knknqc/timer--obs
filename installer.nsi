; ===============================================================
;  Confidence Monitor - NSIS Installer Script
; ===============================================================

!define PLUGIN_NAME     "Confidence Monitor"
!ifndef PLUGIN_VERSION
  !define PLUGIN_VERSION  "1.0.0"
!endif
!define PLUGIN_AUTHOR   "Confidence Monitor"
!define PLUGIN_DLL      "confidence-monitor.dll"
!define PLUGIN_DIR      "confidence-monitor"
!define OBS_PLUGIN_PATH "$APPDATA\obs-studio\plugins"
!define UNINSTALL_KEY   "Software\Microsoft\Windows\CurrentVersion\Uninstall\ConfidenceMonitor"

Name            "${PLUGIN_NAME} ${PLUGIN_VERSION}"
OutFile         "confidence-monitor-setup.exe"
InstallDir      "${OBS_PLUGIN_PATH}\${PLUGIN_DIR}"
InstallDirRegKey HKCU "${UNINSTALL_KEY}" "InstallLocation"

!include "MUI2.nsh"
!include "LogicLib.nsh"

!define MUI_ABORTWARNING

!define MUI_WELCOMEPAGE_TITLE   "Confidence Monitor ${PLUGIN_VERSION}"
!define MUI_WELCOMEPAGE_TEXT    "This wizard will install the Confidence Monitor plugin for OBS Studio.$\r$\n$\r$\nA native timer dock in OBS - black background, automatic colors, built-in sound alerts.$\r$\n$\r$\nClick Next to continue."

!define MUI_FINISHPAGE_TITLE    "Installation complete!"
!define MUI_FINISHPAGE_TEXT     "Confidence Monitor is installed.$\r$\n$\r$\nRestart OBS Studio, then go to:$\r$\n  View > Docks > Confidence Monitor$\r$\n$\r$\nEnjoy your stream!"
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT "Launch OBS Studio"
!define MUI_FINISHPAGE_RUN_FUNCTION LaunchOBS

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "English"

VIProductVersion "${PLUGIN_VERSION}.0"
VIAddVersionKey /LANG=1036 "ProductName"     "${PLUGIN_NAME}"
VIAddVersionKey /LANG=1036 "ProductVersion"  "${PLUGIN_VERSION}"
VIAddVersionKey /LANG=1036 "CompanyName"     "${PLUGIN_AUTHOR}"
VIAddVersionKey /LANG=1036 "FileDescription" "OBS Studio Plugin - ${PLUGIN_NAME}"
VIAddVersionKey /LANG=1036 "FileVersion"     "${PLUGIN_VERSION}"
VIAddVersionKey /LANG=1036 "LegalCopyright"  "MIT License"

RequestExecutionLevel user

Function .onInit
    IfFileExists "$PROGRAMFILES64\obs-studio\bin\64bit\obs64.exe" obs_found
    IfFileExists "$PROGRAMFILES\obs-studio\bin\64bit\obs64.exe" obs_found
    MessageBox MB_YESNO|MB_ICONEXCLAMATION "OBS Studio was not detected on this PC.$\r$\nContinue anyway?" IDYES obs_found
    Abort
    obs_found:
    FindWindow $0 "Qt5QWindowIcon" "OBS 3"
    FindWindow $1 "Qt6QWindowIcon" "OBS 3"
    ${If} $0 != 0
    ${OrIf} $1 != 0
        MessageBox MB_OK|MB_ICONWARNING "OBS Studio is currently running.$\r$\nClose OBS before continuing."
        Abort
    ${EndIf}
FunctionEnd

Section "Main plugin" SecMain
    SectionIn RO
    SetOutPath "$INSTDIR\bin\64bit"
    File "build\Release\${PLUGIN_DLL}"
    SetOutPath "$INSTDIR\data\locale"
    File "data\locale\en-US.ini"
    SetOutPath "$INSTDIR\data"
    File "data\alert.wav"
    WriteRegStr   HKCU "${UNINSTALL_KEY}" "DisplayName"     "${PLUGIN_NAME}"
    WriteRegStr   HKCU "${UNINSTALL_KEY}" "DisplayVersion"  "${PLUGIN_VERSION}"
    WriteRegStr   HKCU "${UNINSTALL_KEY}" "Publisher"       "${PLUGIN_AUTHOR}"
    WriteRegStr   HKCU "${UNINSTALL_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr   HKCU "${UNINSTALL_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegDWORD HKCU "${UNINSTALL_KEY}" "NoModify"        1
    WriteRegDWORD HKCU "${UNINSTALL_KEY}" "NoRepair"        1
    WriteUninstaller "$INSTDIR\uninstall.exe"
    CreateDirectory "$SMPROGRAMS\${PLUGIN_NAME}"
    CreateShortcut  "$SMPROGRAMS\${PLUGIN_NAME}\Uninstall.lnk" "$INSTDIR\uninstall.exe"
SectionEnd

Section "Alert sound" SecSound
    SetOutPath "$INSTDIR\data"
    File "data\alert.wav"
SectionEnd

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecMain}  "Main plugin - OBS dock with timers."
    !insertmacro MUI_DESCRIPTION_TEXT ${SecSound} "Alert sound .wav file."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

Function LaunchOBS
    IfFileExists "$PROGRAMFILES64\obs-studio\bin\64bit\obs64.exe" 0 +2
        Exec '"$PROGRAMFILES64\obs-studio\bin\64bit\obs64.exe"'
    IfFileExists "$PROGRAMFILES\obs-studio\bin\64bit\obs64.exe" 0 +2
        Exec '"$PROGRAMFILES\obs-studio\bin\64bit\obs64.exe"'
FunctionEnd

Section "Uninstall"
    Delete "$INSTDIR\bin\64bit\${PLUGIN_DLL}"
    Delete "$INSTDIR\data\locale\en-US.ini"
    Delete "$INSTDIR\data\alert.wav"
    Delete "$INSTDIR\uninstall.exe"
    RMDir  "$INSTDIR\bin\64bit"
    RMDir  "$INSTDIR\bin"
    RMDir  "$INSTDIR\data\locale"
    RMDir  "$INSTDIR\data"
    RMDir  "$INSTDIR"
    DeleteRegKey HKCU "${UNINSTALL_KEY}"
    Delete "$SMPROGRAMS\${PLUGIN_NAME}\Uninstall.lnk"
    RMDir  "$SMPROGRAMS\${PLUGIN_NAME}"
    MessageBox MB_YESNO "Also delete your saved timer settings?" IDNO skip_settings
        DeleteRegKey HKCU "Software\ConfidenceMonitor"
    skip_settings:
SectionEnd
