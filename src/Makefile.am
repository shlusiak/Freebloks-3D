AM_CXXFLAGS=-std=gnu++17 -O3
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
	gameserver.cpp gameserver.h \
	stone.cpp stone.h \
	board.cpp board.h \
	player.cpp player.h \
	network.cpp network.h \
	turn.h \
	turnpool.cpp turnpool.h \
	ai.cpp ai.h \
	timer.cpp timer.h \
	constants.cpp constants.h \
	logger.cpp logger.h

freebloks_benchmark_SOURCES = \
	benchmark.cpp \
	game.cpp game.h \
	gameserver.cpp gameserver.h \
	gameclient.cpp gameclient.h \
	stone.cpp stone.h \
	board.cpp board.h \
	player.cpp player.h \
	network.cpp network.h \
	turn.h \
	turnpool.cpp turnpool.h \
	ai.cpp ai.h \
	timer.cpp timer.h \
	constants.cpp constants.h \
	logger.cpp logger.h

freebloks_SOURCES = \
	gui.cpp gui.h \
	freebloks.cpp \
	timer.cpp timer.h \
	window.cpp window.h icon.h \
	player.cpp player.h \
	board.cpp board.h \
	stone.cpp stone.h \
	game.cpp game.h \
	gameserver.cpp gameserver.h \
	gameclient.cpp gameclient.h \
	guigameclient.cpp guigameclient.h \
	network.cpp network.h \
	turnpool.cpp turnpool.h \
	turn.h \
	ai.cpp ai.h \
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
	gameclient.cpp gameclient.h \
	game.cpp game.h \
	stone.cpp stone.h \
	board.cpp board.h \
	player.cpp player.h \
	network.cpp network.h \
	turn.h \
	turnpool.cpp turnpool.h \
	ai.cpp ai.h \
	timer.cpp timer.h \
	constants.cpp constants.h

textures = font.bmp wall.bmp ground.bmp field.bmp
EXTRA_DIST = Freebloks.dsp Freebloks.dsw Freebloks.sln Freebloks.vcproj dedicated.vcproj dedicated.dsp \
			Freebloks.ico Freebloks.rc freebloks.png freebloks.desktop \
			resource.h $(textures)

if WITH_GUI
nobase_dist_pkgdata_DATA = $(textures)
icondir = $(datadir)/pixmaps
desktopdir = $(datadir)/applications
dist_icon_DATA = freebloks.png
dist_desktop_DATA = freebloks.desktop
endif




