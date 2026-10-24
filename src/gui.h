/**
 * gui.h
 * Author: Sascha Hlusiak
 *
 * The main class of the GUI. Holds the CWindow, reacts to user
 * events and renders the whole lot. Not to be underestimated!
 **/

#ifndef _WINDOW_INCLUDED_
#define _WINDOW_INCLUDED_

#include "timer.h"
#include "window.h"
#include "stoneeffect.h"
#include "widgets.h"
#include "glfont.h"
#include "guigameclient.h"
#include "options.h"

class CStone;
class CIntro;
class CChatBox;


// This is how big a stone is in world coordinates (the radius, not the diameter)
const double stone_size=0.45;
// This is the height of the bevel on the board, so a stone is twice as high
const double bevel_height=0.15;
// The direction of our directional light source. Needed among others for shadows
const float light0_pos[]    = {2.5f, 5.0f, -2.0f, 0.0f};

const char * const COLOR_NAME[4]={"Blue","Yellow","Red","Green"};


/**
 * The GUI class, effectively the heart of the program
 **/
class CGUI
{
private:
	CWindow * window;

	// A timer for high-precision timing of rendering and animations
	CTimer timer;

	// Linked list for the 2D widgets, i.e. the GUI controls
	CWidgetPane widgets;

	// The chat box always exists
	CChatBox *chatbox;

	// Subwindows that run in parallel to the toolbar should be added here
	CWidget *subchilds;

	// Pointers to important, dynamic widgets
	CFrame *playerframe;
	CStaticText *playertext;

	// Pointer to the 2D main menu, if visible
	CDialog *menu;

	// Effects and animations of stones go into this linked list
	CStoneEffect *effects;

	// The intro object, if it is currently playing.
	CIntro *intro;

	CTexture texture_ground,texture_wall,texture_field;

	TOptions options;

	// Current view angles around the Y and X axes and distance of the camera from the centre
	double angx,angy,camera_distance;

	// Last mouse position
	int lastx,lasty;

	// Coordinate of the last mouse click. Needed for right-click to mirror OR rotate
	int clickx,clicky;

	// Current X and Y coordinate of the selected stone on the board, in board coordinates
	int selectedx,selectedy;

	// Is the move selected on the board allowed?
	char selected_allowed;

	// Currently selected stone, or -1 for no stone
	int current_stone;

	// Time counter for animations
	double anim;

	// Stone currently under the mouse pointer. It is highlighted
	int hovered_stone;

	// Arrow (rotation, mirroring) currently selected with the mouse
	int selected_arrow;

	CGLFont *font;

	// DisplayLists
	GLuint displayLists;

	CButton *hint,*undo;

	// Start-up parameters
	struct PARAMS
	{
		char* remotehost;
		int remoteport;
		bool firststart,intro;
		int humans,threads;
	}startupParams;
public:
	CGUI();
	~CGUI();
	
	// Main method, called from main() in freebloks.cpp.
	void run();

	// Print the help text
	void printHelp()const;

	// Parse the parameters
	bool parseCmdLine(int argc,char **argv);

	// Animates and renders, but only once mintime seconds have passed since the last time.
	bool checkPoint(double mintime=0.0);

	// CWindow calls this when a TMouseEvent occurs. Handled here by the GUI
	void processMouseEvent(TMouseEvent *e);

	void processKeyEvent(unsigned short key);

	// CGameClient reports here that the current player has changed
	void newCurrentPlayer(int player);

	// Set the rotation around the Y axis
	void setAngy(double a) { angy=a; }

	// Set the zoom factor
	void setZoom(double z) { camera_distance=z; }
private:
	void initGL();

	void createDisplayLists();

	void startFirstGame();

	void render();

	// Animate everything by a time difference of elapsed since the last frame
	void execute(double elapsed);
	
	// Checks which object the user is pointing at with the mouse (window coordinate x/y) and sets the corresponding variables.
	void testSelection(int x,int y);

	// true if the given square, in board coordinates, belongs to the current selection on the board, i.e. the user wants to place a stone there
	bool isInSelection(int x,int y)const;

	void renderCube(double x1,double z1,double x2,double z2,double y1,double y2)const;

	// Renders all of the player's stones right in front of the viewer
	void renderPlayerStones(int player,double alpha,bool selectionbuffer)const;

	// Renders a single stone from the player's set, with its arrows if needed
	void renderPlayerStone(int player,CStone* stone,int stone_number,bool current,bool hovered,double alpha,bool selectionbuffer)const;

	void renderPlayerStoneArrows(int player,CStone* stone,int stone_number,bool selectionbuffer)const;

	// Renders exactly one arrow at the origin of the current coordinate system
	void renderArrow(bool doublearrow,bool selected)const;

	void renderQuad(double size)const;

	// Ends the intro, e.g. on a mouse click or key press
	void stopIntro();

	// Shows the move the hard AI would play now
	void showHint();
public:
	// The actual game (the client), connected or not
	CGUIGameClient * game;

	// Renders a single stone at x/y in board coordinates, with colour and alpha
	void renderStone(int x,int y,int color,float alpha,bool doublesided)const;

	// Renders a single stone of the given colour and transparency at the origin of the coordinate system
	void renderStone(int color,float alpha,bool selectionbuffer)const;

	// Renders, starting at the origin, the shadow volume of a stone, projected along the direction of the light
	void renderStoneShadow(float lightdir[3])const;

	// Renders the base under the board (e.g. a wooden table)
	void renderGround()const;

	// Renders the board (without stones), optionally highlighting possible squares in colour
	void renderField(bool withgame=true)const;
	void renderField(bool withgame,int w,int h)const;

	// Adds a CStoneEffect to the linked list. Called by CGameClient after a computer has made a move.
	void addEffect(CStoneEffect *effect) { effects->add(effect); }

	// Adds a (fully modal) subwindow to the widgets!
	void addSubChild(CWidget *w,bool modal=true) { if (modal)widgets.addSubChild(w); else subchilds->addSubChild(w);}

	// Returns the world coordinates of the player's stone that floats in front of the camera.
	// Depends on the viewing angles. Also used by CStoneRollEffect
	void getPlayerStonePos(int player,int stone,double *dx,double *dy,double *dz)const;

	bool startSingleplayerGame(GAMEMODE gamemode,int localplayers,int diff,int width,int height,int8 stone_numbers[],int ai_threads);
	bool startMultiplayerGame(GAMEMODE gamemode,int localplayers,int diff,int width,int height,int8 stone_numbers[],int ai_threads, const char* name);
	bool joinMultiplayerGame(const char *host,int port,int localplayers, const char* name);

	void showIntro();

	// Appends the given text message received from the network to the CChatBox
	void addChatMessage(int client,const char* text);

	// If the chat box is sticky, it does NOT hide automatically after 7 s.
	void setChatBoxSticky(bool sticky);

	TOptions* getOptions() { return &options; }

	void setCurrentStone(int s) { current_stone=s; selected_allowed=false; }

	void setHintEnable(bool b);
};


#endif
