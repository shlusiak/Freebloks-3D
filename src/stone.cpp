#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "stone.h"
#include "board.h"

TSingleStone Shape::get_field(const int y, const int x, const int mirrored, const int rotation) const {
	int nx=x,ny=y;
	if (mirrored == 0){
		if (rotation == 0){
			nx = y;
			ny = x;
		} else if (rotation == 1){
			nx = size-1-x;
			ny = y;
		} else if (rotation == 2){
			nx = size-1-y;
			ny = size-1-x;
		} else if (rotation == 3){
			nx = x;
			ny = size-1-y;
		} else error_exit("Invalid rotation counter", 15); //debug
	} else {
		if (rotation == 0){
			nx = size-1-y;
			ny = x;
		} else if (rotation == 1){
			nx = x;
			ny = y;
		} else if (rotation == 2){
			nx = y;
			ny = size-1-x;
		} else
		if (rotation == 3) {
			nx = size-1-x;
			ny = size-1-y;
		} else error_exit("Invalid rotation counter", 15); //debug
	}

	return STONE_FIELD[shape][nx][ny];
}



void CStone::init(const int shape_no){
	shape = Shape(shape_no);
	orientation = Orientation();
	m_available = 1;
}

int CStone::calculate_possible_turns_in_position(const CBoard& spiel, const int playernumber, const int fieldY, const int fieldX) const {
	int mirror_max;
	int count = 0;

	if (shape.mirrorable == MIRRORABLE_IMPORTANT)
	    mirror_max = 1;
	else
	    mirror_max = 0;

	for (int mirror = 0; mirror <= mirror_max; mirror++){
		for (int rotate = 0; rotate < shape.rotateable; rotate++){

			for (int x = 0; x < shape.size; x++){
				for (int y = 0; y < shape.size; y++){
					if (shape.get_field(y, x, mirror, rotate) == STONE_FIELD_ALLOWED) {
						if (spiel.is_valid_turn(shape, playernumber, fieldY-y, fieldX-x, mirror, rotate)) {
							count++;
						}
					}
				}
			}
		}
	}

	return count;
}
