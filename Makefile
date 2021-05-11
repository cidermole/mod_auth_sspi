# Set APACHEDIR env value to your APACHE HTTPD 2.4 compiled root
# Set PLATSDKDIR env value to your Windows SDK root
# Set MSVCDIR to your Visual Studio\VC subfolder

APACHEDIR=C:\repos\httpd
PLATSDKDIR=C:\Program Files (x86)\Windows Kits\8.0
MSVCDIR=C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC

LIBAPR=libapr-1.lib
LIBAPRUTIL=libaprutil-1.lib

AP_INCLUDES=\
	/I "$(APACHEDIR)\include" /I "$(APACHEDIR)\srclib\apr\include"\
	/I "$(APACHEDIR)\srclib\apr-util\include" /I "$(APACHEDIR)\os\win32"
	
AP_LIBPATH=\
	/LIBPATH:"$(APACHEDIR)\Release"\
	/LIBPATH:"$(APACHEDIR)\srclib\apr\Release"\
	/LIBPATH:"$(APACHEDIR)\srclib\apr-util\Release"

SRCDIR=src
BINDIR=bin

!ifndef DEBUG
DEBUG=0
!endif

CC=cl
CFLAGS=/nologo /W3 /WX
RCFLAGS=/nologo
LD=link
LDFLAGS=/nologo
DEFINES=/D WIN32

INCLUDES=/I include $(AP_INCLUDES) /I "$(PLATSDKDIR)\include" /I "$(MSVCDIR)\include"
LIBPATH=$(AP_LIBPATH) /LIBPATH:"$(PLATSDKDIR)\lib" /LIBPATH:"$(MSVCDIR)\lib"
LIBRARIES=libhttpd.lib $(LIBAPR) $(LIBAPRUTIL) kernel32.lib advapi32.lib ole32.lib

!if ($(DEBUG) != 0)
OBJDIR=Debug
CFLAGS=$(CFLAGS) /LDd /MTd /Od /Z7
LDFLAGS=$(LDFLAGS) /debug
!else
OBJDIR=Release
CFLAGS=$(CFLAGS) /LD /MT /Ot /Ox /Oi /Oy /Ob2 /GF /Gy
LDFLAGS=$(LDFLAGS) /release /opt:ref /opt:icf,16
!endif

DLL_BASE_ADDRESS=0x6ED00000

OBJECTS=$(OBJDIR)\mod_authnz_sspi.obj $(OBJDIR)\mod_authnz_sspi_authentication.obj\
	$(OBJDIR)\mod_authnz_sspi_authorization.obj $(OBJDIR)\mod_authnz_sspi_interface.obj\
	$(OBJDIR)\mod_authnz_sspi.res

OUTFILE=$(BINDIR)\mod_authnz_sspi.so
MAPFILE=$(BINDIR)\mod_authnz_sspi.map

SSPIPKGS=$(BINDIR)\sspipkgs.exe

dist: clean all
	-@del $(BINDIR)\*.exp $(BINDIR)\*.lib 2>NUL
	-@rd /s /q $(OBJDIR) 2>NUL

all: $(OUTFILE) $(SSPIPKGS)

$(OUTFILE): dirs $(OBJECTS)
	$(LD) $(LDFLAGS) /noassembly /DLL /BASE:$(DLL_BASE_ADDRESS) $(LIBPATH) $(OBJECTS) $(LIBRARIES) /OUT:$@ /MAP:$(MAPFILE)

$(SSPIPKGS): dirs $(OBJDIR)\sspipkgs.obj
	$(LD) $(LDFLAGS) $(LIBPATH) $(LIBRARIES) $(OBJDIR)\sspipkgs.obj /OUT:$@

{$(SRCDIR)}.c{$(OBJDIR)}.obj:
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) /c %CD%\$< /Fo$@

{$(SRCDIR)}.rc{$(OBJDIR)}.res:
	$(RC) $(RCFLAGS) $(INCLUDES) $(DEFINES) /fo $@ %CD%\$<
	
dirs:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	@if not exist $(BINDIR) mkdir $(BINDIR)

clean:
	-@rd /s /q $(OBJDIR) 2>NUL
	-@rd /s /q $(BINDIR) 2>NUL

