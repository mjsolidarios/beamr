; Windows installer for the beamr receiver. Built in CI:
;   makensis /DVERSION=0.1.0 /DSOURCE=<deployed folder> /DOUTFILE=<setup.exe> beamr.nsi
; Installs for the current user only, so no administrator rights are needed.

!ifndef VERSION
  !define VERSION "0.0.0"
!endif
!ifndef SOURCE
  !error "Pass /DSOURCE=<folder with beamr-receiver.exe and its DLLs>"
!endif
!ifndef OUTFILE
  !define OUTFILE "beamr-receiver-setup.exe"
!endif

!define APP "beamr"
!define PUBLISHER "Mark Joseph Solidarios"
!define UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\beamr-receiver"

Unicode true
Name "${APP}"
OutFile "${OUTFILE}"
InstallDir "$LOCALAPPDATA\Programs\beamr"
InstallDirRegKey HKCU "Software\beamr" "InstallDir"
RequestExecutionLevel user
SetCompressor /SOLID lzma

!include "MUI2.nsh"
!define MUI_ICON "..\..\receiver-desktop\platform\beamr.ico"
!define MUI_UNICON "..\..\receiver-desktop\platform\beamr.ico"
!define MUI_FINISHPAGE_RUN "$INSTDIR\beamr-receiver.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Start beamr"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\..\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

VIProductVersion "${VERSION}.0"
VIAddVersionKey "ProductName" "${APP}"
VIAddVersionKey "FileDescription" "beamr receiver installer"
VIAddVersionKey "ProductVersion" "${VERSION}"
VIAddVersionKey "LegalCopyright" "MIT License"

Section "beamr" SecMain
  SectionIn RO
  ; A running copy would hold its files open.
  nsExec::Exec 'taskkill /IM beamr-receiver.exe /F'
  SetOutPath "$INSTDIR"
  File /r "${SOURCE}\*.*"
  WriteUninstaller "$INSTDIR\uninstall.exe"

  CreateShortcut "$SMPROGRAMS\beamr.lnk" "$INSTDIR\beamr-receiver.exe"

  WriteRegStr HKCU "Software\beamr" "InstallDir" "$INSTDIR"
  WriteRegStr HKCU "${UNINSTALL_KEY}" "DisplayName" "${APP}"
  WriteRegStr HKCU "${UNINSTALL_KEY}" "DisplayVersion" "${VERSION}"
  WriteRegStr HKCU "${UNINSTALL_KEY}" "Publisher" "${PUBLISHER}"
  WriteRegStr HKCU "${UNINSTALL_KEY}" "DisplayIcon" "$INSTDIR\beamr-receiver.exe"
  WriteRegStr HKCU "${UNINSTALL_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "${UNINSTALL_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKCU "${UNINSTALL_KEY}" "URLInfoAbout" "https://github.com/mjsolidarios/beamr"
  WriteRegDWORD HKCU "${UNINSTALL_KEY}" "NoModify" 1
  WriteRegDWORD HKCU "${UNINSTALL_KEY}" "NoRepair" 1
SectionEnd

Section "Uninstall"
  nsExec::Exec 'taskkill /IM beamr-receiver.exe /F'
  Delete "$SMPROGRAMS\beamr.lnk"
  ; Start at sign-in, if it was turned on.
  DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "beamr"
  RMDir /r "$INSTDIR"
  DeleteRegKey HKCU "${UNINSTALL_KEY}"
  ; Only our own value: the app keeps its settings (trusted phones and so
  ; on) under Software\beamr too, and a reinstall should find them.
  DeleteRegValue HKCU "Software\beamr" "InstallDir"
SectionEnd
