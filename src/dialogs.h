/*
**dialogs.h
**autor: Alex Besstschastnich
*/


#ifndef __DIALOGS_H_INCLUDED_
#define __DIALOGS_H_INCLUDED_

#include "widgets.h"

class CMainMenu:public CDialog
{
private:
	CGUI* GUI;
public:
	CMainMenu(CGUI* gui,bool time);
	virtual int processMouseEvent(TMouseEvent *event);	
};

class CNewGameAdvancedDialog;

class CNewGameDialog:public CDialog
{
private:
	CGUI* GUI;
	CCheckBox* playerMode2;
	CCheckBox* playerMode3;
	CCheckBox* playerMode4;
	CCheckBox* playerMode5;
	CCheckBox* playerMode6;
	CCheckBox* diffEasy;
	CCheckBox* diffNormal;
	CCheckBox* diffHard;
	int playermode;
	int kindOfDiff;
	CCheckBox *blue,*red,*yellow,*green;
	CTextEdit *nameField;
	int size_x,size_y;
	int numberOfStones[5];
	CNewGameAdvancedDialog* advanced;
	bool multiplayer;
	CSpinBox* multithreading;
public:
	CNewGameDialog(CGUI* gui,bool multiplayer);
	virtual int processMouseEvent(TMouseEvent *event);
};

class CNewGameAdvancedDialog:public CDialog
{
public:
	CSpinBox* size_x;
	CSpinBox* size_y;
	CSpinBox* monominoes;
	CSpinBox* dominoes;
	CSpinBox* trominoes;
	CSpinBox* tetrominoes;
	CSpinBox* pentominoes;

	int i_trominoes,i_tetrominoes,i_pentominoes;

	void addTrominoes(int value);
	void addTetrominoes(int value);
	void addPentominoes(int value);
	void checkCheckBoxes();
public:
	CNewGameAdvancedDialog(int sizex,int sizey,int numberOfStones[]);
	virtual int processMouseEvent(TMouseEvent *event);
	bool checkConsistency();
	virtual void execute(double elapsed);
};

class CConnectToMPlayer:public CDialog
{
private:
	CGUI* GUI;	
	CSpinBox* portField;
	CCheckBox *blue,*red,*yellow,*green;
	CTextEdit* serverField, *nameField;
	int max_local;
public:
	CConnectToMPlayer(CGUI* gui);
	virtual int processMouseEvent(TMouseEvent *event);
};


#endif
