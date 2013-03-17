@echo off

:: ������ ��� ���������� ������� iNES 1.0
:: ���������������� ��� ���� ("as is"), ��������� ����� ���������

:: �������� ����������
:: �� ���������� DelayedExpansion, �.�. ������ ������ ���������� "!"
:: � ����� �����
Verify Extensions 2> nul
SetLocal EnableExtensions

If ErrorLevel 1 GoTo End

Echo iNES sort 1>&2
Echo Version: 0.1 1>&2
Echo. 1>&2

If "%~1" == "" (
 PushD "%CD%"
) Else (
 PushD "%~1"
)

Set VPNES_SCRIPT_PATH=%~dp0
Set VPNES_SCRIPT_NAME=%~n0
Set VPNES_TARGET_PATH=%CD%

Path %VPNES_SCRIPT_PATH%;%PATH%

Call :Tool rominfo.exe
:: ���������� ��������� 7z ��� ���������� �������
Call :Tool 7z.exe

:: ���� ���������� �� �����������

Set VPNES_DIR_LEVEL=0
Call :NewScriptPrefix
Set VPNES_TREE=\

Echo File was generated by iNES sorter v0.1 > "%VPNES_TARGET_PATH%\list.txt"
Echo. >> "%VPNES_TARGET_PATH%\list.txt"
Call :GoToFolder "%VPNES_TARGET_PATH%"

Echo. 1>&2
Echo %VPNES_PREFIX% All done 1>&2
Echo. 1>&2
Echo %VPNES_PREFIX% List file: %VPNES_TARGET_PATH%\list.txt 1>&2

PopD

Pause>nul

EndLocal
GoTo :EOF

:: ��������� �����
:GoToFolder

Set /A VPNES_DIR_LEVEL=%VPNES_DIR_LEVEL%+1
Call :NewScriptPrefix

Echo %VPNES_PREFIX% Entering directory: %1 1>&2
PushD "%~1"

:: ������� ������������ ��� *.nes ����� 
Call :ProcessNESROMs

:: ������������ ������ *.zip *.7z *.rar
Call :ProcessArchive *.zip *.7z *.rar

:: ������������ ��������������
For /F "delims=" %%A In ('dir /o /ad /b') Do (
 Call :ConCatTree "%%~nxA\"
 Call :GoToFolder "%%~fA"
 Set VPNES_TREE=%VPNES_TREE%
)

Echo %VPNES_PREFIX% Leaving directory: %1 1>&2
PopD

Set /A VPNES_DIR_LEVEL=%VPNES_DIR_LEVEL%-1
Call :NewScriptPrefix

GoTo :EOF

:: ��������� *.nes ������
:ProcessNESROMs

For %%A In (*.nes) Do (
 Call :ProcessROM "%%~fA"
)

GoTo :EOF

:: ��������� ���������� � �����
:ProcessROM

Call :ConCat %VPNES_TREE% "%~nx1"
Set VPNES_ROMFILE=%Result%

Echo %VPNES_PREFIX% Found ROM: %VPNES_ROMFILE% 1>&2
Call :Command rominfo "%~1"
If Not ErrorLevel 0 (
 Call :ShowError rominfo
 GoTo :EOF
)

Echo %VPNES_ROMFILE% #%ErrorLevel% >> "%VPNES_TARGET_PATH%\list.txt"
rominfo "%~1" 2>> "%VPNES_TARGET_PATH%\list.txt"

GoTo :EOF

:: ��������� �������
:ProcessArchive

For %%A In (%*) Do (
 Echo %VPNES_PREFIX% Found Archive: "%%~fA" 1>&2
 MkDir "%%~fA-temp"
 Call :CommandAndCheck 7z e "%%~fA" -o"%%~fA-temp" -r
 Call :ConCatTree "<%%~nxA>\"
 Call :GoToFolder "%%~fA-temp"
 Call :CommandAndCheck RD /S /Q "%%~fA-temp"
 Set VPNES_TREE=%VPNES_TREE%
)

GoTo :EOF

:: �������
:NewScriptPrefix

If "%VPNES_DIR_LEVEL%" == "0" (
 Set VPNES_PREFIX=%VPNES_SCRIPT_NAME%:
) Else (
 Set VPNES_PREFIX=%VPNES_SCRIPT_NAME%[%VPNES_DIR_LEVEL%]:
)

GoTo :EOF

:: ����� ��������� � ���������� ���������
:Tool

If "%~$PATH:1" == "" (
 GoTo Tool_e
)

Echo tool: %~$PATH:1 1>&2

GoTo :EOF

:Tool_e

Echo.
:Tool_l
Set /p ToolP="Enter path to %~1: "
Call :EnsureNoQuotes %ToolP%
For /F "delims=" %%A In ("%Result%") Do (
 Set ToolP=%%~fA
)
If Not Exist "%ToolP%\%~1" (
 GoTo Tool_l
)
Echo.
Path %ToolP%;%PATH%
Set ToolP=
Call :Tool %*

GoTo :EOF

:: �������� ������
:ShowError

Set PreMsg=
If "%~1" == "" (
 Set PreMsg=%VPNES_PREFIX%
) Else (
 Set PreMsg=%~n1:
)
Echo %PreMsg% Error %ErrorLevel% 1>&2

GoTo :EOF

:: ���������� ���������� �������
:EnsureNoQuotes

Set Result=

:EnsureNoQuotes_l1

If "%~1" == "" GoTo EnsureNoQuotes_l2

Set Result=%Result% %~1
Shift
GoTo EnsureNoQuotes_l1

:EnsureNoQuotes_l2

If Not "%Result%" == "" Set Result=%Result:~1%

GoTo :EOF

:: ������� ������ � ��������� � VPNES_TREE
:ConCatTree

Call :ConCat %VPNES_TREE% %1
Set VPNES_TREE=%Result%

GoTo :EOF

:: ������� ��� ������
:ConCat

Set Result="%~1%~2"

GoTo :EOF

:: ��������
:Command

:: ����� ������
Copy nul nul > nul

Echo %* 1>&2
%*

GoTo :EOF

:: ��������� � ��������� �� ������
:CommandAndCheck

Call :Command %*
If ErrorLevel 1 GoTo ShowError %1

GoTo :EOF

:End

Echo Unsupported COMMAND.COM