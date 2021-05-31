AM_CXXFLAGS=-std=gnu++17
freebloks_CXXFLAGS=$(AM_CXXFLAGS) -DDATA_PREFIX=\"$(datadir)/freebloks/\"

bin_PROGRAMS =
if WITH_DEDICATED
    bin_PROGRAMS += freebloks_dedicated
endif
if WITH_GUI
    bin_PROGRAMS += freebloks
endif
if WITH_CLIENT
    bin_PROGRAMS += freebloks_client
endif
if WITH_BENCHMARK
    bin_PROGRAMS += freebloks_benchmark
endif


freebloks_dedicated_SOURCES = \
	dedicated.cpp \
	game.cpp game.h \
	spielserver.cpp spielserver.h \
	stone.cpp stone.h \
	board.cpp board.h \
	player.cpp player.h \
	network.cpp network.h \
	turn.h \
	turnpool.cpp turnpool.h \
	ki.cpp ki.h \
	timer.cpp timer.h \
	constants.cpp constants.h \
	logger.cpp logger.h

freebloks_benchmark_SOURCES = \
	benchmark.cpp \
	game.cpp game.h \
	spielserver.cpp spielserver.h \
	spielclient.cpp spielclient.h \
	stone.cpp stone.h \
	board.cpp board.h \
	player.cpp player.h \
	network.cpp network.h \
	turn.h \
	turnpool.cpp turnpool.h \
	ki.cpp ki.h \
	timer.cpp timer.h \
	constants.cpp constants.h \
	logger.cpp logger.h

freebloks_SOURCES = \
	gui.cpp gui.h \
	freebloks.cpp \
	timer.cpp timer.h \
	window.cpp window.h \
	player.cpp player.h \
	board.cpp board.h \
	stone.cpp stone.h \
	game.cpp game.h \
	spielserver.cpp spielserver.h \
	spielclient.cpp spielclient.h \
	guispielclient.cpp guispielclient.h \
	network.cpp network.h \
	turnpool.cpp turnpool.h \
	turn.h \
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


freebloks_client_SOURCES = \
	client.cpp \
	spielclient.cpp spielclient.h \
	game.cpp game.h \
	stone.cpp stone.h \
	board.cpp board.h \
	player.cpp player.h \
	network.cpp network.h \
	turn.h \
	turnpool.cpp turnpool.h \
	ki.cpp ki.h \
	timer.cpp timer.h \
	constants.cpp constants.h

textures = font.bmp wall.bmp ground.bmp field.bmp
EXTRA_DIST = Freebloks.dsp Freebloks.dsw Freebloks.sln Freebloks.vcproj dedicated.vcproj dedicated.dsp \
			Freebloks.ico Freebloks.rc freebloks.png freebloks.xpm freebloks.desktop \
			resource.h $(textures)

if WITH_GUI
nobase_dist_pkgdata_DATA = $(textures)
icondir = $(datadir)/pixmaps
desktopdir = $(datadir)/applications
dist_icon_DATA = freebloks.png
dist_desktop_DATA = freebloks.desktop
endif




