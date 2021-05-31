/*
**dialogs.cpp
**autor: Alex Besstschastnich
**
**Beschreibung:
**Design des Men�s und der Untermen�s.
**Alle Steuerelemente haben eine ID, damit man auf die Ereignisse dem entsprechend 
**reagieren kann.
*/


#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif
#ifdef HAVE_UNISTD_H
  #include <unistd.h>
#endif

#include "widgets.h"
#include "dialogs.h"
#include "string.h"
#include "gui.h"
#include "options.h"
#include "game.h"
#include "about.h"
#include "helpbox.h"


/* Standardserver zum Verbinden */
const char* DEFAULT_SERVER="blokus.saschahlusiak.de";


static const int maxNumberStones=10;

/* ...[0] entspricht Anzahl "einer" Steine , ...[1] entspricht Anzahl "zweier" Steine usw. */
/* im SinglePlayer Game */
static int numberOfStones[5]={1,1,1,1,1};

/* Multithreading in der KI verwenden, oder nicht */
static int ki_multithreading=2;

/* Ausgewaehlter Server */
static char* mp_oldserver=nullptr;
static const char* oldname="";

/* Groesse des Feldes */
static int size_x=20,size_y=20;



/**
 *Hauptmen� leitet sich von CDialog ab
 *es werden einige Buttons erstellt
 **/
CMainMenu::CMainMenu(CGUI* gui,bool time)
:CDialog(500,380,"Main Menu")
{
	CButton* cont = new CButton(125,50,250,25,1,this,"Continue");
	if(time)
		cont->setEnabled(false);
	else
		cont->setEnabled(true);
	addChild(cont);	
	addChild(new CButton(125,100,250,25,1002,this,"New Singleplayer Game"));
	addChild(new CButton(125,140,120,25,1003,this,"Host Game"));
	addChild(new CButton(255,140,120,25,1004,this,"Join Game"));
	addChild(new CButton(125,190,250,25,1005,this,"Options"));
	addChild(new CButton(125,230,250,25,1006,this,"Help"));
	addChild(new CButton(125,270,120,25,5,this,"Intro"));
	addChild(new CButton(255,270,120,25,1007,this,"About"));
	addChild(new CButton(125,320,250,25,1008,this,"Quit Game"));

	GUI = gui;
}

/**
 *Funktion zur Bearbeitung der ID's der Buttons nur im Hauptfenster des Men�s.
 *Es werden haupts�chlich weitere Fenster aufgamacht.
 **/
int CMainMenu::processMouseEvent(TMouseEvent *event)
{
	int r=CDialog::processMouseEvent(event);
	switch (r)
	{
	case 1002:
		/* Dialog wird f�r den SinglePlayer Modus erstellt */
		addSubChild(new CNewGameDialog(GUI,false));
		break;
	case 1003:
		/* ...die Namen sprechen ja f�r sich.. */
		addSubChild(new CNewGameDialog(GUI,true));
		break;
	case 1004:
		addSubChild(new CConnectToMPlayer(GUI));
		break;
	case 1005:
		addSubChild(new COptionsDialog(GUI));
		break;
	case 1006:
		addSubChild(new CHelpBox());
		break;
	case 1007:
		addSubChild(new CAboutBox());
		break;
	case 1008:
		/* Dieser return-Wert wird in der Klasse CDialogs bearbeitet
		  und bedeutet das Beenden des Spiels ---> Quit Button */
		return 2;
		
	default: return r;
	}
	return 0;
}



/**
 *Dialog f�r den Single Player Modus
 *man kann den Spielmodus, Spielst�rke, Anzahl lokaler Spieler, und Feldgr�sse �ndern
 **/
CNewGameDialog::CNewGameDialog(CGUI* gui,bool multiplayer)
:CDialog(370,330,"Singleplayer Settings")
{	
	const char* text;
	int i;

	this->multiplayer=multiplayer;
	if (multiplayer)setCaption("Multiplayer Settings");

	setColor(0.1,0.6,0.0);
	if (multiplayer)setSize(w,h+30);

	if (multiplayer) {
		text = "Your name";
		addChild(new CStaticText(30, 51, text, this));
		text = oldname;
		nameField = new CTextEdit(150, 50, 190, 20, 15002, oldname, false, this);
		addChild(nameField);
		nameField->setFocus();
	} else
		nameField = nullptr;


	addChild(new CStaticText(30,h-250,"Mode",this));
	playerMode5 = new CCheckBox(150,h-300,156,20,5004,"Blokus Duo",false,this);
	playerMode6 = new CCheckBox(150,h-280,156,20,5005,"Blokus Junior",false,this);
	playerMode2 = new CCheckBox(150,h-260,156,20,5001,"2 Player 2 Colors",false,this);
	playerMode3 = new CCheckBox(150,h-240,156,20,5003,"2 Player 4 Colors",false,this);
	playerMode4 = new CCheckBox(150,h-220,156,20,5002,"4 Player 4 Colors",true,this);

	addChild(playerMode2);
	addChild(playerMode3);
	addChild(playerMode4);
	addChild(playerMode5);
	addChild(playerMode6);
	playerMode2->addCheckBox(playerMode3);
	playerMode2->addCheckBox(playerMode4);
	playerMode2->addCheckBox(playerMode5);
	playerMode2->addCheckBox(playerMode6);

	i = rand() % 4;
	addChild(new CStaticText(30, h-178, "Your colors", this));
	addChild(yellow = new CCheckBox(150, h-190, 70, 20, 15010, "Yellow", i == 0, this));
	addChild(red = new CCheckBox(255, h-190, 70, 20, 15011, "Red", i == 1, this));
	addChild(blue = new CCheckBox(150, h-170, 70, 20, 15012, "Blue", i == 2, this));
	addChild(green = new CCheckBox(255, h-170, 70, 20, 15013, "Green", i == 3, this));

	text = "Difficulty";
	addChild(new CStaticText(30,h-120,text,this));

	diffEasy	= new CCheckBox(150,h-140,76,20,5020,"easy",false,this);
	diffNormal	= new CCheckBox(150,h-120,76,20,5021,"normal",true,this);
	diffHard	= new CCheckBox(150,h-100,76,20,5022,"hard",false,this);
	addChild(diffEasy);
	addChild(diffNormal);
	addChild(diffHard);
	diffEasy->addCheckBox(diffNormal);
	diffEasy->addCheckBox(diffHard);


	for (i=0;i<sizeof(numberOfStones)/sizeof(numberOfStones[0]);i++)
		this->numberOfStones[i]=::numberOfStones[i];
	addChild(new CStaticText(30,h-70,"Use Threads",this));
	multithreading=new CSpinBox(150,h-73,40,20,5011,1,8,::ki_multithreading,this);
	addChild(multithreading);

	addChild(new CButton(10,h-35,90,25,2000,this,"Advanced"));
	addChild(new CButton(w-160,h-35,70,25,1009,this,"Play"));
	addChild(new CButton(w-80,h-35,70,25,1,this,"Cancel"));
	
	this->size_x=::size_x;
	this->size_y=::size_y;

	playermode = 4;
	kindOfDiff = KI_MEDIUM;

	GUI = gui;
}

int CNewGameDialog::processMouseEvent(TMouseEvent *event)
{
	int r=CDialog::processMouseEvent(event);
	switch (r)
	{
	case 2000:
		addSubChild(advanced=new CNewGameAdvancedDialog(size_x,size_y,numberOfStones));
		break;
	case 2001:
		this->numberOfStones[0]=advanced->einer->getValue();
		this->numberOfStones[1]=advanced->zweier->getValue();
		if (advanced->dreier)this->numberOfStones[2]=advanced->dreier->getValue();
		if (advanced->vierer)this->numberOfStones[3]=advanced->vierer->getValue();
		if (advanced->fuenfer)this->numberOfStones[4]=advanced->fuenfer->getValue();
		size_x=advanced->size_x->getValue();
		size_y=advanced->size_y->getValue();
		break;
	case 5001: /* 2 player 2 color */
		playermode = 2;
		blue->setEnabled(true);
		red->setEnabled(true);
		yellow->setEnabled(false);
		green->setEnabled(false);
		yellow->setCheck(false);
		green->setCheck(false);
		break;
	case 5002:	/* 4 players */
		playermode = 4;
		blue->setEnabled(true);
		red->setEnabled(true);
		yellow->setEnabled(true);
		green->setEnabled(true);
		break;
	case 5003:	/* 2 player 4 colors */
		playermode = 3;
		blue->setEnabled(true);
		red->setEnabled(false);
		red->setCheck(false);
		yellow->setEnabled(true);
		green->setEnabled(false);
		green->setCheck(false);
		break;
	case 5004: /* duo */
		playermode = 5;
		blue->setEnabled(true);
		red->setEnabled(true);
		yellow->setEnabled(false);
		green->setEnabled(false);
		yellow->setCheck(false);
		green->setCheck(false);
		size_x = 14;
		size_y = 14;
		break;
	case 5005: /* junior */
		playermode = 6;
		blue->setEnabled(true);
		red->setEnabled(true);
		yellow->setEnabled(false);
		green->setEnabled(false);
		yellow->setCheck(false);
		green->setCheck(false);
		size_x = 14;
		size_y = 14;
		break;
	case 5013:
	case 5015:
		break;
	case 5020:
		kindOfDiff = KI_EASY;
		break;
	case 5021:
		kindOfDiff = KI_MEDIUM;
		break;
	case 5022:
		kindOfDiff = KI_HARD;
		break;
	case 1009:{
		GAMEMODE m=GAMEMODE_4_COLORS_4_PLAYERS;
		int min;

		min=(size_x<size_y)?size_x:size_y;

		if (playermode==2)m=GAMEMODE_2_COLORS_2_PLAYERS;
		if (playermode==3)m=GAMEMODE_4_COLORS_2_PLAYERS;
		if (playermode==5)m=GAMEMODE_DUO;
		if (playermode==6)m=GAMEMODE_JUNIOR;

		if ((numberOfStones[0]==0)&&
			(numberOfStones[1]==0)&&
			(min<=3 || numberOfStones[2]==0)&&
			(min<=4 || numberOfStones[3]==0)&&
			(min<=5 || numberOfStones[4]==0))
		{
			addSubChild(advanced=new CNewGameAdvancedDialog(size_x,size_y,numberOfStones));
			advanced->checkConsistency();
		}else{
			::numberOfStones[0]=this->numberOfStones[0];
			::numberOfStones[1]=this->numberOfStones[1];
			::numberOfStones[2]=this->numberOfStones[2];
			::numberOfStones[3]=this->numberOfStones[3];
			::numberOfStones[4]=this->numberOfStones[4];
			::ki_multithreading=multithreading->getValue();

			::size_x=this->size_x;
			::size_y=this->size_y;

			if (min<=5)this->numberOfStones[4]=0;
			if (min<=4)this->numberOfStones[3]=0;
			if (min<=3)this->numberOfStones[2]=0;

			const char *name = nullptr;
			if (multiplayer) {
				name = nameField->getText();
				if (strcmp(name, "") == 0)
					name = nullptr;
			}
			int players = 0;
			if (blue->getCheck()) players |= 0x01;
			if (yellow->getCheck()) players |= 0x02;
			if (red->getCheck()) players |= 0x04;
			if (green->getCheck()) players |= 0x08;

			int8 einer = numberOfStones[0];
			int8 zweier = numberOfStones[1];
			int8 dreier = numberOfStones[2];
			int8 vierer = numberOfStones[3];
			int8 fuenfer = numberOfStones[4];
			int8 a_default[STONE_COUNT_ALL_SHAPES] = {
					einer,
					zweier,
					dreier, dreier,
					vierer, vierer, vierer, vierer, vierer,
					fuenfer, fuenfer, fuenfer, fuenfer, fuenfer, fuenfer, fuenfer, fuenfer, fuenfer, fuenfer, fuenfer, fuenfer,
			};
			int8 a_junior[STONE_COUNT_ALL_SHAPES] = {
					2,
					2,
					2, 2,
					2, 2, 2, 2, 2,
					2, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 0
			};

			int8* a;
			if (m == GAMEMODE_JUNIOR)
				a = a_junior;
			else
				a = a_default;

			if (!multiplayer)
				GUI->startSingleplayerGame(m, players, kindOfDiff, size_x, size_y,a,::ki_multithreading);
			else {
				GUI->startMultiplayerGame(m, players, kindOfDiff, size_x, size_y,a,::ki_multithreading, name);
				oldname = strdup(nameField->getText());
			}
			return 1;
		}
		}
		break;
	default:
		return r;
	}
	
	return 0;
}





CNewGameAdvancedDialog::CNewGameAdvancedDialog(int sizex,int sizey,int numberOfStones[])
:CDialog(340,260,"Advanced Settings")
{
	int min=(sizex<sizey)?sizex:sizey;
	setColor(0.4,0.6,1.0);

	addChild(new CButton((w-70)/2-40,h-40,70,25,1013,this,"Okay"));
	addChild(new CButton(w/2+5,h-40,70,25,1014,this,"Cancel"));

	addChild(new CStaticText(30,40,"Field size (w x h)",this));

	size_x = new CSpinBox(225,40,45,20,5013,3,30,sizex,this,false);
	addChild(size_x);
	addChild(new CStaticText(271,43,"x",this));
	size_y = new CSpinBox(280,40,45,20,5015,3,30,sizey,this);
	addChild(size_y);

	addChild(new CStaticText(30,70,"Stones with one squares",this));
	einer=new CSpinBox(260,70,45,20,5100,0,maxNumberStones,numberOfStones[0],this);
	addChild(einer);

	addChild(new CStaticText(30,100,"Stones with two squares",this));
	zweier=new CSpinBox(260,100,45,20,5102,0,maxNumberStones,numberOfStones[1],this);
	addChild(zweier);

	i_dreier=numberOfStones[2];
	i_vierer=numberOfStones[3];
	i_fuenfer=numberOfStones[4];

	addChild(new CStaticText(30,130,"Stones with three squares",this));
	if (min>3)
		addDreier(numberOfStones[2]);
	else dreier=nullptr;

	addChild(new CStaticText(30,160,"Stones with four squares",this));
	if (min>4)
		addVierer(numberOfStones[3]);
	else vierer=nullptr;

	addChild(new CStaticText(30,190,"Stones with five squares",this));
	if (min>5)
		addFuenfer(numberOfStones[4]);
	else fuenfer=nullptr;
}

int CNewGameAdvancedDialog::processMouseEvent(TMouseEvent *event)
{
	int r=CDialog::processMouseEvent(event);
	switch (r)
	{
	case 1013:
		/* OK wurde gedr�ckt.. */
		/* Wenn die Anzahl aller Steine auf Null steht, kommt eine Warnung */
		/* Wenn die Anzahl der Steine ungleich null ist,
		 * �bernehme die Werte, der Spinbxes */
		if (checkConsistency())
		{
			close();
			// Wert zurueck geben, dass Werte uebernommen werden sollen
			return 2001;
		}
		break;
	case 1014:
		/* Cancel wurde gedr�ckt, also einfach schliessen */
		close();
		break;

	case 5015:
	case 5013:
		checkCheckBoxes();
		break;
	default:
		return r;
	}
	return 0;
}

void CNewGameAdvancedDialog::checkCheckBoxes()
{
	int min=size_x->getValue();
	if (size_y->getValue()<min)min=size_y->getValue();
	
	if (min<=3)
	{
		if (dreier)
		{
			i_dreier=dreier->getValue();
			dreier->remove();
			dreier=nullptr;
		}
	}else if (!dreier)addDreier(i_dreier);
	if (min<=4)
	{
		if (vierer)
		{
			i_vierer=vierer->getValue();
			vierer->remove();
			vierer=nullptr;
		}
	}else if (!vierer)addVierer(i_vierer);
	if (min<=5)
	{
		if (fuenfer)
		{
			i_fuenfer=fuenfer->getValue();
			fuenfer->remove();
			fuenfer=nullptr;
		}
	}else if (!fuenfer)addFuenfer(i_fuenfer);
}

bool CNewGameAdvancedDialog::checkConsistency()
{
	if(!einer->getValue() && !zweier->getValue() && 
		(!dreier || !dreier->getValue()) && 
		(!vierer || !vierer->getValue()) && 
		(!fuenfer || !fuenfer->getValue()) )
		addSubChild(new CMessageBox(300,100,"Dumb user","Please choose at least one stone!!!"));
	else return true;
	return false;
}

void CNewGameAdvancedDialog::addDreier(int value)
{
	dreier=new CSpinBox(260,130,45,20,5104,0,maxNumberStones,value,this);
	addChild(dreier);
}

void CNewGameAdvancedDialog::addVierer(int value)
{
	vierer=new CSpinBox(260,160,45,20,5106,0,maxNumberStones,value,this);
	addChild(vierer);
}

void CNewGameAdvancedDialog::addFuenfer(int value)
{
	fuenfer=new CSpinBox(260,190,45,20,5108,0,maxNumberStones,value,this);
	addChild(fuenfer);
}	

void CNewGameAdvancedDialog::execute(double elapsed)
{
	CDialog::execute(elapsed);
	if (size_x->committed() || size_y->committed())
	{
		checkCheckBoxes();
		size_x->unsetCommitted();
		size_y->unsetCommitted();
	}
}









CConnectToMPlayer::CConnectToMPlayer(CGUI* gui)
:CDialog(350,170,"Connect to Multiplayer Game")
{
	const char* text;
	setColor(0.1,0.6,0.0);

	addChild(new CButton((w-70)/2-40,h-35,70,25,1011,this,"Join"));
	addChild(new CButton(w/2+5,h-35,70,25,1,this,"Cancel"));

	text = oldname;
	addChild(new CStaticText(10, 53, "Name:", this));
	nameField = new CTextEdit(60, 51, 90, 20, 15002, oldname, false, this);
	addChild(nameField);
	nameField->setFocus();

	int i = rand() % 4;
	addChild(yellow = new CCheckBox(170, 40, 70, 20, 15010, "Yellow", i == 0, this));
	addChild(red = new CCheckBox(270, 40, 70, 20, 15011, "Red", i == 1, this));
	addChild(blue = new CCheckBox(170, 62, 70, 20, 15012, "Blue", i == 2, this));
	addChild(green = new CCheckBox(270, 62, 70, 20, 15013, "Green", i == 3, this));

	text=mp_oldserver;
	if (text==nullptr)text=DEFAULT_SERVER;
	serverField = new CTextEdit(10,100,260,20,15000,text,false,this);
	addChild(serverField);
	portField= new CSpinBox(280,100,60,20,15001,1023,65535,59995,this);
	addChild(portField);

	GUI = gui;
}

int CConnectToMPlayer::processMouseEvent(TMouseEvent *event)
{
	int r=CDialog::processMouseEvent(event);
	switch (r)
	{
	case 1011: {
		const char *name = nameField->getText();
		if (strcmp(name, "") == 0)
			name = nullptr;
		int players = 0;
		if (blue->getCheck()) players |= 0x01;
		if (yellow->getCheck()) players |= 0x02;
		if (red->getCheck()) players |= 0x04;
		if (green->getCheck()) players |= 0x08;

		if (GUI->joinMultiplayerGame(serverField->getText(), portField->getValue(), players, name))
		{
			if (mp_oldserver)free(mp_oldserver);
			mp_oldserver=strdup(serverField->getText());
			oldname = strdup(nameField->getText());
			return 1;
		}
		break;
	}
	default:
		break;
	}
	return 0;
}


