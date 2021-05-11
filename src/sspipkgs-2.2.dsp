# Microsoft Developer Studio Project File - Name="sspipkgs" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=sspipkgs - Win32 Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "sspipkgs-2.2.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "sspipkgs-2.2.mak" CFG="sspipkgs - Win32 Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "sspipkgs - Win32 Release" (basierend auf  "Win32 (x86) Console Application")
!MESSAGE "sspipkgs - Win32 Debug" (basierend auf  "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "sspipkgs - Win32 Release"

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
# ADD BASE CPP /nologo /MD /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "APR_DECLARE_STATIC" /D "APU_DECLARE_STATIC" /FD /c
# ADD CPP /nologo /MD /W3 /O2 /I "../../../httpd-2.2.2/srclib/apr/include" /I "../../../httpd-2.2.2/srclib/apr-util/include" /D "NDEBUG" /D "WIN32" /D "_CONSOLE" /D "APR_DECLARE_STATIC" /D "APU_DECLARE_STATIC" /Fd"Release/sspipkgs" /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib advapi32.lib wsock32.lib ws2_32.lib /nologo /subsystem:console /map /machine:I386
# ADD LINK32 libhttpd.lib libapr-1.lib libaprutil-1.lib kernel32.lib advapi32.lib wsock32.lib ws2_32.lib /nologo /subsystem:console /map /machine:I386 /out:"../bin/sspipkgs.exe" /libpath:"../../../httpd-2.2.2/Release" /libpath:"../../../httpd-2.2.2/srclib/apr/Release" /libpath:"../../../httpd-2.2.2/srclib/apr-util/Release"

!ELSEIF  "$(CFG)" == "sspipkgs - Win32 Debug"

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
# ADD BASE CPP /nologo /MDd /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "APR_DECLARE_STATIC" /D "APU_DECLARE_STATIC" /FD /c
# ADD CPP /nologo /MDd /W3 /GX /Zi /Od /I "../../../httpd-2.2.2/srclib/apr/include" /I "../../../httpd-2.2.2/srclib/apr-util/include" /D "_DEBUG" /D "WIN32" /D "_CONSOLE" /D "APR_DECLARE_STATIC" /D "APU_DECLARE_STATIC" /Fd"Debug/sspipkgs" /FD /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib advapi32.lib wsock32.lib ws2_32.lib /nologo /subsystem:console /incremental:no /map /debug /machine:I386
# ADD LINK32 libhttpd.lib libapr-1.lib libaprutil-1.lib kernel32.lib advapi32.lib wsock32.lib ws2_32.lib /nologo /subsystem:console /incremental:no /map /debug /machine:I386 /out:"../bin_dbg/sspipkgs.exe" /libpath:"../../../httpd-2.2.2/Debug" /libpath:"../../../httpd-2.2.2/srclib/apr/Debug" /libpath:"../../../httpd-2.2.2/srclib/apr-util/Debug"

!ENDIF 

# Begin Target

# Name "sspipkgs - Win32 Release"
# Name "sspipkgs - Win32 Debug"
# Begin Source File

SOURCE=.\sspipkgs.c
# End Source File
# Begin Source File

SOURCE=.\sspipkgs.rc
# End Source File
# End Target
# End Project
