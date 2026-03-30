; ═══════════════════════════════════════════════════════════════
;  Confidence Monitor — NSIS Installer Script
;  Génère un .exe installeur professionnel pour OBS Studio
;
;  Prérequis : NSIS ≥ 3.0  (https://nsis.sourceforge.io)
;  Commande  : makensis installer.nsi
; ═══════════════════════════════════════════════════════════════

!define PLUGIN_NAME     "Confidence Monitor"
!define PLUGIN_VERSION  "1.0.0"
!define PLUGIN_AUTHOR   "Confidence Monitor"
!define PLUGIN_DLL      "confidence-monitor.dll"
!define PLUGIN_DIR      "confidence-monitor"
!define OBS_PLUGIN_PATH "$APPDATA\obs-studio\plugins"
!define UNINSTALL_KEY   "Software\Microsoft\Windows\CurrentVersion\Uninstall\ConfidenceMonitor"

; ── Métadonnées de l'installeur ──────────────────────────────
Name            "${PLUGIN_NAME} ${PLUGIN_VERSION}"
OutFile         "confidence-monitor-setup.exe"
InstallDir      "${OBS_PLUGIN_PATH}\${PLUGIN_DIR}"
InstallDirRegKey HKCU "${UNINSTALL_KEY}" "InstallLocation"

; Interface moderne
!include "MUI2.nsh"
!include "LogicLib.nsh"

; ── Icône & UI ───────────────────────────────────────────────
!define MUI_ABORTWARNING
!define MUI_ICON   "assets\icon.ico"
!define MUI_UNICON "assets\icon.ico"

!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP   "assets\header.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP "assets\welcome.bmp"

!define MUI_WELCOMEPAGE_TITLE    "Confidence Monitor ${PLUGIN_VERSION}"
!define MUI_WELCOMEPAGE_TEXT     "Ce wizard va installer le plugin Confidence Monitor pour OBS Studio.$\r$\n$\r$\nUn dock de timers natif dans OBS — fond noir, couleurs automatiques, alertes sonores intégrées.$\r$\n$\r$\nCliquez Suivant pour continuer."

!define MUI_FINISHPAGE_TITLE     "Installation terminée !"
!define MUI_FINISHPAGE_TEXT      "Confidence Monitor est installé.$\r$\n$\r$\nRedémarre OBS Studio, puis va dans :$\r$\n  Affichage → Docks → Confidence Monitor$\r$\n$\r$\nBonne diffusion !"
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT  "Lancer OBS Studio"
!define MUI_FINISHPAGE_RUN_FUNCTION LaunchOBS

; ── Pages ────────────────────────────────────────────────────
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; ── Langue ───────────────────────────────────────────────────
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "English"

; ── Infos version Windows ────────────────────────────────────
VIProductVersion "${PLUGIN_VERSION}.0"
VIAddVersionKey /LANG=1036 "ProductName"      "${PLUGIN_NAME}"
VIAddVersionKey /LANG=1036 "ProductVersion"   "${PLUGIN_VERSION}"
VIAddVersionKey /LANG=1036 "CompanyName"      "${PLUGIN_AUTHOR}"
VIAddVersionKey /LANG=1036 "FileDescription"  "OBS Studio Plugin — ${PLUGIN_NAME}"
VIAddVersionKey /LANG=1036 "FileVersion"      "${PLUGIN_VERSION}"
VIAddVersionKey /LANG=1036 "LegalCopyright"   "MIT License"

; Permissions (pas besoin d'admin — install dans %APPDATA%)
RequestExecutionLevel user

; ═══════════════════════════════════════════════════════════════
;  Détection OBS avant installation
; ═══════════════════════════════════════════════════════════════

Function .onInit
    ; Vérifier qu'OBS est installé
    IfFileExists "$PROGRAMFILES64\obs-studio\bin\64bit\obs64.exe" obs_found
    IfFileExists "$PROGRAMFILES\obs-studio\bin\64bit\obs64.exe" obs_found

    MessageBox MB_YESNO|MB_ICONEXCLAMATION \
        "OBS Studio n'a pas été détecté sur ce PC.$\r$\n$\r$\nVoulez-vous continuer quand même ?" \
        IDYES obs_found
    Abort

    obs_found:
    ; Vérifier que OBS n'est pas en cours d'exécution
    FindWindow $0 "Qt5QWindowIcon" "OBS 3"
    FindWindow $1 "Qt6QWindowIcon" "OBS 3"
    ${If} $0 != 0
    ${OrIf} $1 != 0
        MessageBox MB_OK|MB_ICONWARNING \
            "OBS Studio est actuellement ouvert.$\r$\nFerme OBS avant de continuer l'installation."
        Abort
    ${EndIf}
FunctionEnd

; ═══════════════════════════════════════════════════════════════
;  Composants
; ═══════════════════════════════════════════════════════════════

Section "Plugin principal" SecMain
    SectionIn RO   ; obligatoire, non décoché

    SetOutPath "$INSTDIR\bin\64bit"
    File "build\Release\${PLUGIN_DLL}"

    SetOutPath "$INSTDIR\data\locale"
    File "data\locale\en-US.ini"

    ; ── Son intégré (copié pour référence — déjà compilé dans le .dll) ──
    SetOutPath "$INSTDIR\data"
    File "data\alert.wav"

    ; ── Infos désinstallation ─────────────────────────────────
    WriteRegStr   HKCU "${UNINSTALL_KEY}" "DisplayName"     "${PLUGIN_NAME}"
    WriteRegStr   HKCU "${UNINSTALL_KEY}" "DisplayVersion"  "${PLUGIN_VERSION}"
    WriteRegStr   HKCU "${UNINSTALL_KEY}" "Publisher"       "${PLUGIN_AUTHOR}"
    WriteRegStr   HKCU "${UNINSTALL_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr   HKCU "${UNINSTALL_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegDWORD HKCU "${UNINSTALL_KEY}" "NoModify"        1
    WriteRegDWORD HKCU "${UNINSTALL_KEY}" "NoRepair"        1

    WriteUninstaller "$INSTDIR\uninstall.exe"

    ; ── Raccourci dans le menu Démarrer ──────────────────────
    CreateDirectory "$SMPROGRAMS\${PLUGIN_NAME}"
    CreateShortcut  "$SMPROGRAMS\${PLUGIN_NAME}\Désinstaller.lnk" \
                    "$INSTDIR\uninstall.exe"

SectionEnd

Section "Son d'alerte personnalisé" SecSound
    SetOutPath "$INSTDIR\data"
    File "data\alert.wav"
SectionEnd

; ── Descriptions des composants ──────────────────────────────
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecMain}  "Plugin principal — dock OBS avec timers."
    !insertmacro MUI_DESCRIPTION_TEXT ${SecSound} "Fichier son .wav d'alerte (déjà intégré dans le plugin, copie de référence)."
!insertmacro MUI_FUNCTION_DESCRIPTION_END

; ═══════════════════════════════════════════════════════════════
;  Lancer OBS après install (page Finish)
; ═══════════════════════════════════════════════════════════════

Function LaunchOBS
    IfFileExists "$PROGRAMFILES64\obs-studio\bin\64bit\obs64.exe" 0 +2
        Exec '"$PROGRAMFILES64\obs-studio\bin\64bit\obs64.exe"'
    IfFileExists "$PROGRAMFILES\obs-studio\bin\64bit\obs64.exe" 0 +2
        Exec '"$PROGRAMFILES\obs-studio\bin\64bit\obs64.exe"'
FunctionEnd

; ═══════════════════════════════════════════════════════════════
;  Désinstallation
; ═══════════════════════════════════════════════════════════════

Section "Uninstall"
    ; Supprimer le plugin
    Delete "$INSTDIR\bin\64bit\${PLUGIN_DLL}"
    Delete "$INSTDIR\data\locale\en-US.ini"
    Delete "$INSTDIR\data\alert.wav"
    Delete "$INSTDIR\uninstall.exe"

    RMDir  "$INSTDIR\bin\64bit"
    RMDir  "$INSTDIR\bin"
    RMDir  "$INSTDIR\data\locale"
    RMDir  "$INSTDIR\data"
    RMDir  "$INSTDIR"

    ; Nettoyer le registre
    DeleteRegKey HKCU "${UNINSTALL_KEY}"

    ; Nettoyer les raccourcis
    Delete "$SMPROGRAMS\${PLUGIN_NAME}\Désinstaller.lnk"
    RMDir  "$SMPROGRAMS\${PLUGIN_NAME}"

    ; Supprimer les paramètres sauvegardés (optionnel)
    MessageBox MB_YESNO "Supprimer aussi tes paramètres de timers sauvegardés ?" IDNO skip_settings
        DeleteRegKey HKCU "Software\ConfidenceMonitor"
    skip_settings:

SectionEnd
