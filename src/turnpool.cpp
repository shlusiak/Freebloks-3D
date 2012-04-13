#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "turnpool.h"




CTurnpool::CTurnpool(){
	m_head = 0;
	m_tail = 0;
	m_current = 0;
}


CTurnpool::~CTurnpool(){
	delete_all_turns();
}


//add_turn sind fast identisch
void CTurnpool::add_turn(const CTurn* turn){
	CTurn* new_element = new CTurn(turn);
	new_element->set_number(CTurnpool::get_number_of_stored_turns()+1);
	if (0 == m_head){
		m_head = new_element;
		m_tail = new_element;
		m_current = new_element;
	}else{
		m_tail->set_next(new_element);
		m_tail = m_tail->get_next(); //new_element ist get_next;
	}
}


void CTurnpool::add_turn(const int playernumber, const CStone* stone, const int y, const int x){
	CTurn* new_element = new CTurn(CTurnpool::get_number_of_stored_turns()+1, playernumber, stone, y, x);
	if (0 == m_head){
		m_head = new_element;
		m_tail = new_element;
		m_current = new_element;
	}else{
		m_tail->set_next(new_element);
		m_tail = m_tail->get_next(); //new_element ist get_next;
	}
}





void CTurnpool::delete_all_turns(){
	if (m_head) { 
		delete m_head;
	}
	m_tail = 0;
	m_head = 0;
	m_current = 0;
}



const int CTurnpool::get_number_of_stored_turns()const{
	if (0 == m_tail) return 0;
	return m_tail->get_turn_number();
}


CTurn* CTurnpool::get_last_turn(){
	return CTurnpool::m_tail;
}


void CTurnpool::delete_last(){
	#ifdef _DEBUG
		if (CTurnpool::get_number_of_stored_turns() == 0) error_exit("Turnpool ist leer!!... delete_last unsinnig", 31);
	#endif
	
	if (CTurnpool::get_number_of_stored_turns() == 1) {
		CTurnpool::delete_all_turns();
		return;
	}
		
	m_current = m_head;
	while (m_current->get_next() != m_tail){
		m_current = m_current->get_next();
	}
	m_current->set_next(NULL);
	delete m_tail;
	m_tail = m_current;
}


CTurn* CTurnpool::get_turn(int i){
	#ifdef _DEBUG
		if (0 == m_head || i > m_tail->get_turn_number()) error_exit("Turnpool ist leer. Wertrückgabe nicht möglich!", 6); //debug
	#endif
	if (i < m_current->get_turn_number()) m_current = m_head;
	while (i > m_current->get_turn_number()){
		m_current = m_current->get_next();
	}
	return m_current;	
}
