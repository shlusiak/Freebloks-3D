# Microsoft Developer Studio Project File - Name="Freebloks" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=Freebloks - Win32 Debug
!MESSAGE Dies ist kein g�ltiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und f�hren Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "Freebloks.mak".
!MESSAGE 
!MESSAGE Sie k�nnen beim Ausf�hren von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "Freebloks.mak" CFG="Freebloks - Win32 Debug"
!MESSAGE 
!MESSAGE F�r die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "Freebloks - Win32 Release" (basierend auf  "Win32 (x86) Application")
!MESSAGE "Freebloks - Win32 Debug" (basierend auf  "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Freebloks - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /Op /Ob2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib glu32.lib opengl32.lib ws2_32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "Freebloks - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /Fr /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib glu32.lib opengl32.lib ws2_32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "Freebloks - Win32 Release"
# Name "Freebloks - Win32 Debug"
# Begin Group "Quellcodedateien"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\about.cpp
# End Source File
# Begin Source File

SOURCE=.\freebloks.cpp
# End Source File
# Begin Source File

SOURCE=.\Freebloks.rc
# End Source File
# Begin Source File

SOURCE=.\chatbox.cpp
# End Source File
# Begin Source File

SOURCE=.\constants.cpp
# End Source File
# Begin Source File

SOURCE=.\dialogs.cpp
# End Source File
# Begin Source File

SOURCE=.\gamedialogs.cpp
# End Source File
# Begin Source File

SOURCE=.\glfont.cpp
# End Source File
# Begin Source File

SOURCE=.\gui.cpp
# End Source File
# Begin Source File

SOURCE=.\helpbox.cpp
# End Source File
# Begin Source File

SOURCE=.\intro.cpp
# End Source File
# Begin Source File

SOURCE=.\ki.cpp
# End Source File
# Begin Source File

SOURCE=.\logger.cpp
# End Source File
# Begin Source File

SOURCE=.\network.cpp
# End Source File
# Begin Source File

SOURCE=.\options.cpp
# End Source File
# Begin Source File

SOURCE=.\player.cpp
# End Source File
# Begin Source File

SOURCE=.\spiel.cpp
# End Source File
# Begin Source File

SOURCE=.\spielclient.cpp
# End Source File
# Begin Source File

SOURCE=.\spielleiter.cpp
# End Source File
# Begin Source File

SOURCE=.\spielserver.cpp
# End Source File
# Begin Source File

SOURCE=.\stone.cpp
# End Source File
# Begin Source File

SOURCE=.\stoneeffect.cpp
# End Source File
# Begin Source File

SOURCE=.\texture.cpp
# End Source File
# Begin Source File

SOURCE=.\timer.cpp
# End Source File
# Begin Source File

SOURCE=.\turn.cpp
# End Source File
# Begin Source File

SOURCE=.\turnpool.cpp
# End Source File
# Begin Source File

SOURCE=.\widgets.cpp
# End Source File
# Begin Source File

SOURCE=.\window.cpp
# End Source File
# End Group
# Begin Group "Header-Dateien"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\about.h
# End Source File
# Begin Source File

SOURCE=.\chatbox.h
# End Source File
# Begin Source File

SOURCE=.\constants.h
# End Source File
# Begin Source File

SOURCE=.\dialogs.h
# End Source File
# Begin Source File

SOURCE=.\gamedialogs.h
# End Source File
# Begin Source File

SOURCE=.\glfont.h
# End Source File
# Begin Source File

SOURCE=.\gui.h
# End Source File
# Begin Source File

SOURCE=.\helpbox.h
# End Source File
# Begin Source File

SOURCE=.\intro.h
# End Source File
# Begin Source File

SOURCE=.\ki.h
# End Source File
# Begin Source File

SOURCE=.\logger.h
# End Source File
# Begin Source File

SOURCE=.\network.h
# End Source File
# Begin Source File

SOURCE=.\options.h
# End Source File
# Begin Source File

SOURCE=.\player.h
# End Source File
# Begin Source File

SOURCE=.\situation.h
# End Source File
# Begin Source File

SOURCE=.\situationpool.h
# End Source File
# Begin Source File

SOURCE=.\spiel.h
# End Source File
# Begin Source File

SOURCE=.\spielclient.h
# End Source File
# Begin Source File

SOURCE=.\spielleiter.h
# End Source File
# Begin Source File

SOURCE=.\spielserver.h
# End Source File
# Begin Source File

SOURCE=.\stone.h
# End Source File
# Begin Source File

SOURCE=.\stoneeffect.h
# End Source File
# Begin Source File

SOURCE=.\texture.h
# End Source File
# Begin Source File

SOURCE=.\timer.h
# End Source File
# Begin Source File

SOURCE=.\turn.h
# End Source File
# Begin Source File

SOURCE=.\turnpool.h
# End Source File
# Begin Source File

SOURCE=.\widgets.h
# End Source File
# Begin Source File

SOURCE=.\window.h
# End Source File
# End Group
# Begin Group "Ressourcendateien"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\Freebloks.ico
# End Source File
# End Group
# End Target
# End Project
