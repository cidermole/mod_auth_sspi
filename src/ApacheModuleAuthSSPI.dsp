# Microsoft Developer Studio Project File - Name="ApacheModuleAuthSSPI" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=ApacheModuleAuthSSPI - Win32 Debug
!MESSAGE Dies ist kein g�ltiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und f�hren Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "ApacheModuleAuthSSPI.mak".
!MESSAGE 
!MESSAGE Sie k�nnen beim Ausf�hren von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "ApacheModuleAuthSSPI.mak" CFG="ApacheModuleAuthSSPI - Win32 Debug"
!MESSAGE 
!MESSAGE F�r die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "ApacheModuleAuthSSPI - Win32 Release" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE "ApacheModuleAuthSSPI - Win32 Debug" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "ApacheModuleAuthSSPI - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "APACHEMODULEAUTHSSPI_EXPORTS" /YX /FD /c
# ADD CPP /nologo /G6 /MD /W3 /Ox /Ot /Oa /Ow /Og /Op /Ob2 /I "..\..\httpd-2.0.58\srclib\apr\include" /I "..\..\..\httpd-2.0.58\srclib\apr\include" /I "..\..\..\httpd-2.0.58\include" /I "..\..\..\httpd-2.0.58\srclib\apr-util\include" /I "..\..\..\httpd-2.0.58\os\win32" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "APACHEMODULEAUTHSSPI_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc09 /d "NDEBUG"
# ADD RSC /l 0xc09 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 libhttpd.lib libapr.lib libaprutil.lib advapi32.lib ole32.lib /nologo /base:"0x6EEE0000" /dll /map /machine:I386 /out:"../bin/mod_auth_sspi.so" /libpath:"../../../httpd-2.0.58/Release" /libpath:"../../../httpd-2.0.58/srclib/apr/Release" /libpath:"../../../httpd-2.0.58/srclib/apr-util/Release" /release
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "ApacheModuleAuthSSPI - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "APACHEMODULEAUTHSSPI_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /G6 /MDd /W3 /Gm /Zi /Od /I "..\..\..\httpd-2.0.58\srclib\apr\include" /I "..\..\..\httpd-2.0.58\include" /I "..\..\..\httpd-2.0.58\srclib\apr-util\include" /I "..\..\..\httpd-2.0.58\os\win32" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "APACHEMODULEAUTHSSPI_EXPORTS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc09 /d "_DEBUG"
# ADD RSC /l 0xc09 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 libhttpd.lib libapr.lib libaprutil.lib advapi32.lib ole32.lib /nologo /base:"0x6EEE0000" /dll /debug /machine:I386 /out:"../bin_dbg/mod_auth_sspi.so" /pdbtype:sept /libpath:"../../../httpd-2.0.58/Debug" /libpath:"../../../httpd-2.0.58/srclib/apr/Debug" /libpath:"../../../httpd-2.0.58/srclib/apr-util/Debug"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "ApacheModuleAuthSSPI - Win32 Release"
# Name "ApacheModuleAuthSSPI - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\accesscheck.c
# End Source File
# Begin Source File

SOURCE=.\authentication.c
# End Source File
# Begin Source File

SOURCE=.\interface.c
# End Source File
# Begin Source File

SOURCE=.\mod_auth_sspi.c
# End Source File
# Begin Source File

SOURCE=.\mod_auth_sspi.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\mod_auth_sspi.h
# End Source File
# Begin Source File

SOURCE=..\mod_auth_sspi_ver.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
