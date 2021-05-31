#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "board.h"

#include "constants.h"


const int DEFAULT_FIELD_SIZE_X = 20;
const int DEFAULT_FIELD_SIZE_Y = 20;


CBoard::CBoard(){
	CBoard::m_game_field = nullptr;
	CBoard::m_field_size_y = DEFAULT_FIELD_SIZE_Y;
	CBoard::m_field_size_x = DEFAULT_FIELD_SIZE_X;
}

CBoard::CBoard(int width, int height) {
	m_field_size_x = width;
	m_field_size_y = height;
	CBoard::m_game_field = new TSingleField[CBoard::m_field_size_y * CBoard::m_field_size_x];
}


CBoard::CBoard(const int player_team1_1, const int player_team1_2, const int player_team2_1, const int player_team2_2){
	CBoard::m_game_field = nullptr;
	CBoard::m_field_size_y = DEFAULT_FIELD_SIZE_Y;
	CBoard::m_field_size_x = DEFAULT_FIELD_SIZE_X;
	start_new_game(GAMEMODE_4_COLORS_4_PLAYERS);
	CBoard::set_teams(player_team1_1, player_team1_2, player_team2_1, player_team2_2);
}

void CBoard::follow_situation(const CBoard& from_board, const CTurn& turn) {
	memcpy(m_game_field, from_board.get_field_pointer(), m_field_size_x * m_field_size_y);
	memcpy(m_player, from_board.m_player, sizeof(m_player));
	set_stone(turn);
}

int CBoard::get_player_start_x(const int player_number)const{
	switch (player_number) {
	case 0 :
	case 1 : return 0;
	default: return CBoard::m_field_size_x - 1;
	}
}

int CBoard::get_player_start_y(const int player_number)const{
	switch (player_number){
	case 1 :
	case 2 : return 0;
	default: return CBoard::m_field_size_y - 1;
	}
}

void CBoard::set_stone_numbers(int8 stone_numbers[]){
	for (int n = 0 ; n < STONE_COUNT_ALL_SHAPES; n++){  
		for (int p = 0; p < PLAYER_MAX; p++){
			m_player[p].get_stone(n).set_available(stone_numbers[n]);
		}
	}

	CBoard::refresh_player_data();
}

void CBoard::set_stone_numbers(int8 einer, int8 zweier, int8 dreier, int8 vierer, int8 fuenfer) {
	int8 a[STONE_COUNT_ALL_SHAPES] = {
			einer,
			zweier,
			dreier, dreier,
			vierer, vierer, vierer, vierer, vierer,
			fuenfer, fuenfer, fuenfer, fuenfer, fuenfer, fuenfer, fuenfer, fuenfer, fuenfer, fuenfer, fuenfer, fuenfer,
	};
	set_stone_numbers(a);
}

void CBoard::set_teams(int player_team1_1, int player_team1_2, int player_team2_1, int player_team2_2){
	#ifdef _DEBUG 
		for (int p = 0; p < PLAYER_MAX; p++){
			int count = 0;
			if (player_team1_1 == p) count++;
			if (player_team1_2 == p) count++;
			if (player_team2_1 == p) count++;
			if (player_team2_2 == p) count++;
			if (count != 1) error_exit("Invalid parameter", 20);
		}
	#endif

	CBoard::m_player[player_team1_1].set_teammate(player_team1_2);
	CBoard::m_player[player_team1_2].set_teammate(player_team1_1);
	CBoard::m_player[player_team1_1].set_nemesis(player_team2_1);
	CBoard::m_player[player_team1_2].set_nemesis(player_team2_1);

	CBoard::m_player[player_team2_1].set_teammate(player_team2_2);
	CBoard::m_player[player_team2_2].set_teammate(player_team2_1);
	CBoard::m_player[player_team2_1].set_nemesis(player_team1_1);
	CBoard::m_player[player_team2_2].set_nemesis(player_team1_1);
}

CBoard::~CBoard(){
	delete [] CBoard::m_game_field;
}

void CBoard::start_new_game(GAMEMODE game_mode){
	init_field();
	set_seeds(game_mode);
	for (int n = 0; n < PLAYER_MAX; n++){
		m_player[n].init(*this, n);
	}
}

void CBoard::refresh_player_data(){
	for (int n = 0; n < PLAYER_MAX; n++){
		m_player[n].refresh_data(*this);
	}
}


void CBoard::init_field(){
	delete[] CBoard::m_game_field;
	CBoard::m_game_field = new TSingleField[CBoard::m_field_size_y * CBoard::m_field_size_x];
	memset(m_game_field, 0, sizeof(TSingleField) * m_field_size_y * m_field_size_x);
}

void CBoard::set_seeds(GAMEMODE game_mode) {
	#define set_seed(x, y, player) \
		if (get_game_field(player, y, x) == FIELD_FREE) \
			set_game_field(y, x, PLAYER_BIT_ALLOWED[player])
	if (game_mode == GAMEMODE_DUO || game_mode == GAMEMODE_JUNIOR) {
		set_seed(4, m_field_size_y - 5, 0);
		set_seed(m_field_size_x - 5, 4, 2);
	} else {
		for (int p = 0; p < PLAYER_MAX; p++){
			set_seed(get_player_start_x(p), get_player_start_y(p), p);
		}
	}
	#undef set_seed
}

bool CBoard::is_valid_turn(const CStone& stone, int playernumber, int startY, int startX) const {
	bool valid = false;
	TSingleField field_value;

	for (int y = 0; y < stone.get_stone_size(); y++){
		for (int x = 0; x < stone.get_stone_size(); x++){
			if (stone.get_stone_field(y,x) != STONE_FIELD_FREE) {
				if (!is_position_inside_field(y + startY, x + startX))
					return false;

				field_value = get_game_field(playernumber, y + startY , x + startX);
				if (field_value == FIELD_DENIED)
					return false;

				if (field_value == FIELD_ALLOWED)
					valid = true;
			}
		}
	}

	return valid;
}

bool CBoard::is_valid_turn(const CTurn& turn) {
	int playernumber = turn.player;
	CStone& stone = m_player[playernumber].get_stone(turn.stone_number);
	stone.mirror_rotate_to(turn.mirror_count, turn.rotate_count);
	return is_valid_turn(stone, playernumber, turn.y, turn.x);
}

void CBoard::free_game_field(int y, int x){
	set_game_field(y, x, 0);
}

void CBoard::set_single_stone_for_player(const int player_number, const int startY, const int startX){
	CBoard::set_game_field(startY , startX, PLAYER_BIT_HAVE_MIN | player_number);
	for (int y = startY-1; y <= startY+1; y++)if (y>=0 && y<m_field_size_y) {
		for (int x = startX-1; x <= startX+1; x++)if (x>=0 && x<m_field_size_x){
			if (get_game_field(player_number, y, x) != FIELD_DENIED){
				if (y != startY && x != startX){
					CBoard::m_game_field[y * CBoard::m_field_size_x + x] |= PLAYER_BIT_ALLOWED[player_number];
				}else{
					CBoard::m_game_field[y * CBoard::m_field_size_x + x] &= ~PLAYER_BIT_ADDR[player_number];
					CBoard::m_game_field[y * CBoard::m_field_size_x + x] |= PLAYER_BIT_DENIED[player_number];
				}
			}
		}
	}
}

bool CBoard::set_stone(const CTurn& turn){
	int playernumber = turn.player;
	CStone& stone = m_player[playernumber].get_stone(turn.stone_number);
	stone.mirror_rotate_to(turn.mirror_count, turn.rotate_count);
	return set_stone(stone, playernumber, turn.y, turn.x);
}

bool CBoard::set_stone(CStone& stone, int playernumber, int startY, int startX) {
#ifdef _DEBUG
	if (playernumber < 0 || playernumber >= PLAYER_MAX) error_exit("Falsche Spielerzahl", playernumber); //debug
#endif
//	if (is_valid_turn(stone, playernumber, startY, startX) == FIELD_DENIED) return FIELD_DENIED;

	for (int y = 0; y < stone.get_stone_size(); y++){
		for (int x = 0; x < stone.get_stone_size(); x++){
			if (stone.get_stone_field(y,x) != STONE_FIELD_FREE) {
				CBoard::set_single_stone_for_player(playernumber, startY + y, startX + x);
			}
		}
	}

	stone.available_decrement();
	refresh_player_data();

	return true;
}

void CBoard::undo_turn(CTurnPool& turn_pool, GAMEMODE game_mode){
	const CTurn* turn = turn_pool.get_last_turn();
	CStone& stone = CBoard::m_player[turn->player].get_stone(turn->stone_number);
	int x, y;
	stone.mirror_rotate_to(turn->mirror_count, turn->rotate_count);

	#ifdef _DEBUG
		//check valid
		if (turn == nullptr) error_exit("Kein turn", 42);
		for (x = 0; x < stone->get_stone_size(); x++){
			for (y = 0; y < stone->get_stone_size(); y++){
				if (stone->get_stone_field(y, x) != STONE_FIELD_FREE){
					if (CBoard::get_game_field(turn->get_y() + y, turn->get_x() + x) != turn->get_playernumber()) {
						printf("y: %d, x: %d\n", turn->get_y() + y, turn->get_x() +x);
						error_exit("�bergebener Turnpool fehlerhaft (undo turn)", 44);//return false;
					}
				}
			}
		}
	#endif

	//delete stone
	for (x = 0; x < stone.get_stone_size(); x++){
		for (y = 0; y < stone.get_stone_size(); y++){
			if (stone.get_stone_field(y, x) != STONE_FIELD_FREE){
				CBoard::free_game_field(turn->y + y, turn->x + x);
			}
		}
	}

	//redraw gamefield
	for (x = 0; x < CBoard::get_field_size_x(); x++){
		for (y = 0; y < CBoard::get_field_size_y(); y++){
			if (CBoard::get_game_field(y, x) == FIELD_FREE ){
				CBoard::free_game_field(y, x);
			}
		}
	}
	for (x = 0; x < CBoard::get_field_size_x(); x++){
		for (y = 0; y < CBoard::get_field_size_y(); y++){
			if (CBoard::get_game_field(y, x) != FIELD_FREE ){
				CBoard::set_single_stone_for_player(CBoard::get_game_field(y, x), y, x);
			}
		}
	}

	set_seeds(game_mode);
	stone.available_increment();
	refresh_player_data();
	//end redraw
	turn_pool.delete_last();
}
