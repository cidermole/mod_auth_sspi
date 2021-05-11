APACHEDIR=..\httpd-2.0.58
MSVCDIR=\program files\microsoft visual studio\vc98
PLATSDKDIR=\program files\microsoft sdk
MSVCDIR=c:\programme\microsoft visual studio\vc98
PLATSDKDIR=c:\programme\microsoft sdk

SRCDIR=src
OBJDIR=obj
BINDIR=bin

!ifndef DEBUG
DEBUG=0
!endif

CC=cl /nologo
CFLAGS=/G6 /W3 /WX
INCLUDES=/I include\
	 /I "$(APACHEDIR)\include" /I "$(APACHEDIR)\srclib\apr\include"\
	 /I "$(APACHEDIR)\srclib\apr-util\include" /I "$(APACHEDIR)\os\win32"\
	 /I "$(PLATSDKDIR)\include" /I "$(MSVCDIR)\include"
DEFINES=/D WIN32

LD=link /nologo
LDFLAGS=
LIBPATH=/LIBPATH:"$(PLATSDKDIR)\lib"\
	/LIBPATH:"$(MSVCDIR)\lib"
LIBRARIES=$(APACHEDIR)\Release\libhttpd.lib\
	  $(APACHEDIR)\srclib\apr\Release\libapr.lib\
	  $(APACHEDIR)\srclib\apr-util\Release\libaprutil.lib\
	  kernel32.lib advapi32.lib ole32.lib

!if ($(DEBUG) != 0)
CFLAGS=$(CFLAGS) /MDd /Od /Z7
LDFLAGS=$(LDFLAGS) /debug /pdb:none
!else
CFLAGS=$(CFLAGS) /MD /Ot /Og /Oi /Oy /Ob2 /GF /Gy
LDFLAGS=$(LDFLAGS) /release /opt:ref /opt:icf,16
!endif

DLL_BASE_ADDRESS=0x6ED00000

OBJECTS=$(OBJDIR)\mod_auth_sspi.obj $(OBJDIR)\authentication.obj\
	$(OBJDIR)\accesscheck.obj $(OBJDIR)\interface.obj\
	$(OBJDIR)\mod_auth_sspi.res

OUTFILE=$(BINDIR)\mod_auth_sspi.so
MAPFILE=$(BINDIR)\mod_auth_sspi.map

SSPIPKGS=$(BINDIR)\sspipkgs.exe

dist: clean all
	-@del $(BINDIR)\*.exp $(BINDIR)\*.lib 2>NUL
	-@rd /s /q $(OBJDIR) 2>NUL

all: $(OUTFILE) $(SSPIPKGS)

$(OUTFILE): dirs $(OBJECTS)
	$(LD) $(LDFLAGS) /DLL /BASE:$(DLL_BASE_ADDRESS) $(LIBPATH) $(OBJECTS) $(LIBRARIES) /OUT:$@ /MAP:$(MAPFILE)

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

