#ifndef ____CONSTANTS___H_
#define ____CONSTANTS___H_

#ifdef WIN32
	const double M_PI = 3.14159265358979323846264338327950;
#endif


const int AI_PERFECT = 0;
const int AI_HARD = 5;
const int AI_MEDIUM = 50;
const int AI_EASY = 120;

typedef char TSingleStone;
typedef unsigned char TSingleField;



const int PLAYER_MAX   =  4;
const int CLIENTS_MAX  =  8;

const TSingleField PLAYER_BIT_ADDR[PLAYER_MAX] = {
			  3, //0
			 12, //1
			 48, //2
			192  //3
};

const TSingleField PLAYER_BIT_ALLOWED[PLAYER_MAX] = {
			  1, //0
			  4, //1
			 16, //2
			 64  //3
};

const TSingleField PLAYER_BIT_DENIED[PLAYER_MAX] = {
			  2, //0
			  8, //1
			 32, //2
			128  //3
};

const TSingleField PLAYER_BIT_HAVE_MIN = 252;


const int STONE_COUNT_ALL_SHAPES = 21;
const int STONE_SIZE_MAX  =  5;

const int MIRRORABLE_NOT = 0;
const int MIRRORABLE_OPTIONAL = 1;
const int MIRRORABLE_IMPORTANT = 2;

const int ROTATEABLE_NOT   = 1;
const int ROTATEABLE_TWO   = 2;
const int ROTATEABLE_FOUR  = 4;


typedef TSingleStone TStoneField[STONE_SIZE_MAX][STONE_SIZE_MAX];

const TSingleField FIELD_FREE   =  240;
const TSingleField FIELD_ALLOWED=  241;
const TSingleField FIELD_DENIED =  255;

const TSingleStone STONE_FIELD_FREE = 0;
const TSingleStone STONE_FIELD_ALLOWED = 1;

enum GAMEMODE {
	GAMEMODE_2_COLORS_2_PLAYERS,
	GAMEMODE_4_COLORS_2_PLAYERS,
	GAMEMODE_4_COLORS_4_PLAYERS,
	GAMEMODE_DUO,
	GAMEMODE_JUNIOR,
};

/* Field edge limits accepted from the network */
const int FIELD_SIZE_MIN = 4;
const int FIELD_SIZE_MAX = 50;

/**
 * Checks a game mode and field size received from the network.
 * DUO and JUNIOR place their seeds at offset 4 from the field edges, which needs at least 5 fields.
 **/
inline bool is_valid_game_config(int gamemode, int width, int height)
{
	if (gamemode < GAMEMODE_2_COLORS_2_PLAYERS || gamemode > GAMEMODE_JUNIOR) return false;
	if (width < FIELD_SIZE_MIN || width > FIELD_SIZE_MAX) return false;
	if (height < FIELD_SIZE_MIN || height > FIELD_SIZE_MAX) return false;
	if (gamemode == GAMEMODE_DUO || gamemode == GAMEMODE_JUNIOR)
		if (width < 5 || height < 5) return false;
	return true;
}



void error_exit(const char* message, int code);


#endif

