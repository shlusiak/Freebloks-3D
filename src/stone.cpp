#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "stone.h"
#include "board.h"

TSingleStone Shape::get_field(const int y, const int x, const Orientation& orientation) const {
	int nx=x,ny=y;
	if (orientation.mirrored == 0){
		if (orientation.rotation == 0){
			nx = y;
			ny = x;
		} else if (orientation.rotation == 1){
			nx = size-1-x;
			ny = y;
		} else if (orientation.rotation == 2){
			nx = size-1-y;
			ny = size-1-x;
		} else if (orientation.rotation == 3){
			nx = x;
			ny = size-1-y;
		} else error_exit("Invalid rotation counter", 15); //debug
	} else {
		if (orientation.rotation == 0){
			nx = size-1-y;
			ny = x;
		} else if (orientation.rotation == 1){
			nx = x;
			ny = y;
		} else if (orientation.rotation == 2){
			nx = y;
			ny = size-1-x;
		} else
		if (orientation.rotation == 3) {
			nx = size-1-x;
			ny = size-1-y;
		} else error_exit("Invalid rotation counter", 15); //debug
	}

	return STONE_FIELD[shape][nx][ny];
}



void CStone::init(const int shape){
	CStone::m_available = 1;
	CStone::m_shape = shape;
	CStone::m_size = STONE_SIZE[m_shape];
	CStone::m_rotate_counter = 0;
	CStone::m_mirror_counter = 0;
}

TSingleStone CStone::get_stone_field(const int y, const int x, const int mirror, const int rotate) const {
	#ifdef _DEBUG
		if (!is_position_inside_stone(y,x)) error_exit("Stone field mit is_position_inside_stone �berpr�fen!!", 23);
	#endif
	int nx=x,ny=y;
	if (mirror == 0){
		if (rotate == 0){
			nx = y;
			ny = x;
		} else if (rotate == 1){
			nx = m_size-1-x;
			ny = y;
		} else if (rotate == 2){
			nx = m_size-1-y;
			ny = m_size-1-x;
		} else if (rotate == 3){
			nx = x;
			ny = m_size-1-y;
		} else error_exit("unbekannter steinzustand!", 15); //debug
	}else{
		if (rotate == 0){
			nx = m_size-1-y;
			ny = x;
		} else if (rotate == 1){
			nx = x;
			ny = y;
		} else if (rotate == 2){
			nx = y;
			ny = m_size-1-x;
		} else
		if (rotate == 3){
			nx = m_size-1-x;
			ny = m_size-1-y;
		} else error_exit("unbekannter steinzustand!", 15); //debug
	}

	return STONE_FIELD[m_shape][nx][ny];
}

void CStone::rotate_left() {
	m_rotate_counter--;
	if (m_rotate_counter < 0)
		m_rotate_counter += STONE_ROTATEABLE[m_shape];
}

void CStone::rotate_right() {
	m_rotate_counter = (m_rotate_counter + 1) % STONE_ROTATEABLE[m_shape];
// 	if (CStone::m_rotate_counter >= STONE_ROTATEABLE[m_shape]) CStone::m_rotate_counter = 0;
}

void CStone::mirror_over_x() {
	if (STONE_ROTATEABLE[m_shape] == MIRRORABLE_NOT) return;
	m_mirror_counter = (m_mirror_counter + 1) % 2;
	if (m_rotate_counter % 2 == 1)
		m_rotate_counter = (m_rotate_counter + 2) % (STONE_ROTATEABLE[m_shape]);
}

 void CStone::mirror_over_y() {
	if (STONE_ROTATEABLE[m_shape] == MIRRORABLE_NOT) return;
	m_mirror_counter = (m_mirror_counter + 1) % 2;
	if (m_rotate_counter % 2 == 0)
		m_rotate_counter = (m_rotate_counter + 2) % (STONE_ROTATEABLE[m_shape]);
}

int CStone::calculate_possible_turns_in_position(const CBoard& spiel, const int playernumber, const int fieldY, const int fieldX) const {
	int mirror_max;
	int count = 0;

	if (STONE_MIRRORABLE[m_shape] == MIRRORABLE_IMPORTANT)
	    mirror_max = 1;
	else
	    mirror_max = 0;

	for (int mirror = 0; mirror <= mirror_max; mirror++){
		for (int rotate = 0; rotate < STONE_ROTATEABLE[m_shape]; rotate++){

			for (int x = 0; x < STONE_SIZE[m_shape]; x++){
				for (int y = 0; y < STONE_SIZE[m_shape]; y++){
					if (get_stone_field(y, x, mirror, rotate) == STONE_FIELD_ALLOWED) {
						if (spiel.is_valid_turn(*this, playernumber, fieldY-y, fieldX-x)) {
							count++;
						}
					}
				}
			}
		}
	}

	return count;
}
