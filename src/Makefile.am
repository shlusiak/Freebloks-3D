bin_PROGRAMS =
if WITH_DEDICATED
    bin_PROGRAMS += blokus_dedicated
endif
if WITH_GUI
    bin_PROGRAMS += blokus
endif
if WITH_CLIENT
    bin_PROGRAMS += blokus_client
endif


blokus_dedicated_SOURCES = \
dedicated.cpp \
spielleiter.cpp spielleiter.h \
spielserver.cpp spielserver.h \
stone.cpp stone.h \
spiel.cpp spiel.h \
player.cpp player.h \
network.cpp network.h \
turn.cpp turn.h \
turnpool.cpp turnpool.h \
ki.cpp ki.h \
timer.cpp timer.h \
constants.cpp constants.h \
logger.cpp logger.h

blokus_SOURCES = \
gui.cpp gui.h \
blokus.cpp \
timer.cpp timer.h \
window.cpp window.h \
player.cpp player.h \
spiel.cpp spiel.h \
stone.cpp stone.h \
spielleiter.cpp spielleiter.h \
spielserver.cpp spielserver.h \
spielclient.cpp spielclient.h \
guispielclient.cpp guispielclient.h \
network.cpp network.h \
turnpool.cpp turnpool.h \
turn.cpp turn.h \
ki.cpp ki.h \
stoneeffect.cpp stoneeffect.h \
widgets.cpp widgets.h \
texture.cpp texture.h \
glfont.cpp glfont.h \
constants.cpp constants.h \
dialogs.cpp dialogs.h \
intro.cpp intro.h \
chatbox.cpp chatbox.h \
options.cpp options.h \
gamedialogs.cpp gamedialogs.h \
about.cpp about.h \
helpbox.cpp helpbox.h \
logger.cpp logger.h


blokus_client_SOURCES = \
client.cpp \
spielclient.cpp spielclient.h \
spielleiter.cpp spielleiter.h \
stone.cpp stone.h \
spiel.cpp spiel.h \
player.cpp player.h \
network.cpp network.h \
turn.cpp turn.h \
turnpool.cpp turnpool.h \
ki.cpp ki.h \
timer.cpp timer.h \
constants.cpp constants.h

blokus_CXXFLAGS='-DDATA_PREFIX="$(datadir)/blokus/"'

textures = font.bmp wall.bmp ground.bmp field.bmp
EXTRA_DIST = Blokus.dsp Blokus.dsw Blokus.sln Blokus.vcproj dedicated.vcproj dedicated.dsp Blokus.ico Blokus.rc blokus.png blokus.xpm blokus.desktop resource.h $(textures)

if WITH_GUI
    nobase_dist_pkgdata_DATA = $(textures)
#    icondir = $(datadir)/pixmaps
#    utildir = $(datadir)/applications
#    icon_DATA = blokus.png
#    util_DATA = blokus.desktop
endif
