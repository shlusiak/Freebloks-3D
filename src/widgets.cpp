/**
 * widgets.cpp
 * Autor: Sascha Hlusiak
 *
 * 2D Engine fuer die Menues, Klassen fuer Buttons Checkboxes, Dialoge, etc.
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include "widgets.h"
#include "window.h"
#include "glfont.h"
#include "constants.h"
#include "spielclient.h"
#include "gui.h"
#include "options.h"


/**
 * Initialisiert ein CWidget auf Standardwerte. Nimmt (theoretisch) den gesamten
 * Bildschirm in Anspruch
 **/
CWidget::CWidget()
{
	/* Verkettete Listen leeren. Das Widget ist ganz allein auf der Welt. */
	next=child=subchild=0; 
	/* Nimmt erstmal gesamten Bildschirm in Anspruch. Stoert nicht, ausser wenn eine
	   m_id gesetzt ist. */
	x=y=0.0;
	w=640.0;
	h=480.0;
	/* Keine ID */
	m_id=0;
	/* Und Widget ist am Leben und will vorerst nicht entfernt werden. */
	m_deleted=false;
}

/** 
 * CWidget abraeumen. 
 **/
CWidget::~CWidget()
{
	/* Alle nachfolgenden Widgets der verketteten Listen werden rekursiv mit entfernt! */
	if (child)delete child;
	if (subchild)delete subchild;
	if (next)delete next;
}

/**
 * Fuegt ein SubChild der Liste der SubChilds an. Das sind z.B. modale Dialoge.
 * Einfach der verketteten Liste hinten anfuegen, oder das Widget als Kopf benutzen.
 **/
void CWidget::addSubChild(CWidget *sub)
{
	if (subchild)subchild->addSubChild(sub);
	else subchild=sub;
}

/**
 * Defaulthandler fuer execute. elapsed ist die Zeit in sek seit dem letzten Aufruf.
 * Hier werden rekursiv die execute Methoden der Elemente der verketteten Listen aufgerufen.
 **/
void CWidget::execute(double elapsed)
{
	if (subchild)
	{
		/* Wenn das subchild entfernt werden moechte, wird es hier entfernt. */
		if (subchild->deleted())
		{
			/* Das zu entfernende SubChild wird sanft aus der verketteten Liste
			   genommen... */
			CWidget *w=subchild;
			subchild=subchild->subchild;
			w->subchild=0;
			/* ... und als Einziges entfernt. */
			delete w;
		}
		/* Gibt es ein SubChild, execute aufrufen. */
		if (subchild)subchild->execute(elapsed);
	}
	if (child)
	{
		/* Wenn das child entfernt werden will, sanft aus der verketteten Liste nehmen. */
		if (child->deleted())
		{
			CWidget* w=child;
			child=child->child;
			w->child=0;
			/* Child als einziges loeschen. Nachfolger bleiben erhalten. */
			delete w;
		}
		/* Child animieren. */
		if (child)child->execute(elapsed);
	}
	if (next)
	{
		/* Wenn next entfernt werden will, aus der Liste nehmen und sanft loeschen. */
		if (next->deleted())
		{
			CWidget* w=next;
			next=next->next;
			w->next=0;
			delete w;
		}
		/* Das next animieren. */
		if (next)next->execute(elapsed);
	}
}

/**
 * Ein CWidget rendern. 
 * Die Implementierung ruft die render() Methoden von next und child auf.
 * SubChild wird in einem separatem Aufruf gerendert, damit die Z-Order stimmt.
 * selection ist true, wenn das Ding in den Selectionbuffer gerendert werden soll.
 **/
void CWidget::render(bool selection)
{
	/* Im Falle des Selectionbuffers, sofern kein subchild existiert und eine id
	   vergeben ist, rendere ein Quad an die durch x,y,w,h definierte Position, welches
	   getId() benamst wird. */
	if (selection && !subchild && getId()!=0)
	{
		glPushName(4);
		glPushName(getId());
		glBegin(GL_QUADS);
		glColor3f(1,1,1);
		glVertex3d(x,y,0);
		glVertex3d(x,y+h,0);
		glVertex3d(x+w,y+h,0);
		glVertex3d(x+w,y,0);
		glEnd();
		glPopName();
		glPopName();
	}
	/* next und child rendern.
	   NICHT rendern, wenn selection && subchild. */
	if (next && !(subchild && selection))
	{
		next->render(selection);
	}
	if (child&& !(subchild && selection))
	{
		child->render(selection);
	}
}

/**
 * Das subChild rendern, sofern existent.
 * Das geschieht in einem Extraschritt, damit die Z-Reihenfolge richtig bleibt.
 **/
void CWidget::renderSubChild(bool selectionBuffer)
{
	/* Sollten child und next subchilds haben, duerfen diese gerendert werden,
	   sofern NICHT selectionBuffer && subchild, da hier das subchild exklusiv
	   in den SelectionBuffer schreiben darf. Mann ist das kompliziert...
	   Es durfen halt keine Widgets in den SelectionBuffer schreiben, wenn ein
	   modaler Unterdialog existiert. */
	if (child &&(!subchild || !selectionBuffer))child->renderSubChild(selectionBuffer);
	if (next && (!subchild || !selectionBuffer))next->renderSubChild(selectionBuffer);
	/* Das subchild rendern, wenn vorhanden. */
	if (subchild)
	{
		/* Aktuelle Transformation merken (z.B. Dialogverformungen) */
		glPushMatrix();
		glLoadIdentity();
		/* SubChild normal rendern. */
		subchild->render(selectionBuffer);
		glPopMatrix();
		/* Und dann subChilds von subchild rendern. */
		subchild->renderSubChild(selectionBuffer);
	}
}

/**
 * Die GUI teilt mit, dass die Maus sich gerade ueber dem Widget mit der id id befindet.
 **/
void CWidget::processSelection(int id)
{
	/* Ist die Maus garnicht auf einem Widget, ist sie auch nicht ueber this. Also
	   durch mouseLeave darueber informieren. */
	if (getId()!=0 && id==0)mouseLeave();
	/* subchild davon in Kenntnis setzen. */
	if (subchild)subchild->processSelection(id);
	/* child und next in Kenntnis setzen, aber nur wenn kein subchild existiert, oder
	   die Maus ueber keinem Widget ist, damit alle mouseLeave aufrufen koennen. */
	if (child && (!subchild || id==0))child->processSelection(id);
	if (next && (!subchild || id==0))next->processSelection(id);
	/* Wenn das Widget der obersten Ebene angehoert, und die id==getId() ist, darf
	   mouseEnter aufgerufen werden. */
	if (!subchild && id!=0 && id==getId())mouseEnter();
	/* Wenn die Ids nicht stimmen, mouseLeave() aufrufen. */
	if (id!=0 && id!=getId())mouseLeave();
}

/**
 * Tastendruck verarbeiten. key ist der Ascii-Code der Taste.
 * Gibt true zurueck, wenn die Taste verarbeitet wurde, sonst false.
 **/
bool CWidget::processKey(unsigned short key)
{
	/* Wenn ein subchild existiert, Taste verarbeiten, und sofort raus.
	   next und child garnicht erst die Gelegenheit geben, die Taste zu sehen. */
	if (subchild)if (subchild->processKey(key))return true;else return false;
	/* Sonst child und next die Taste verarbeiten lassen. */
	if (child) if (child->processKey(key))return true;
	if (next)if (next->processKey(key))return true;
	/* Die Taste wurde nicht von uns verarbeitet, also false zurueck. */
	return false;
}

/**
 * Ein Mausereignis ist eingetreten (Move, Click, Release, etc.)
 * Standardverarbeitung ist, es an die verketteten Listen durchzureichen.
 * Der Rueckgabewert ist ein Befehl an den Aufrufer, oder 0 wenn das Ereignis nicht
 * Bearbeitet wurd oder kein besonderer Rueckgabewert vorliegt.
 **/
int CWidget::processMouseEvent(TMouseEvent *event)
{
	int r=0;
	/* Wenn subchild, hat das das exklusive Vergnuegen, das Ereignis zu verarbeiten */
	if (subchild)
	{
		r=subchild->processMouseEvent(event);
		if (r!=0 || subchild->handleUI())return r;
	}
	/* Wenn child, darf child es verarbeiten. */
	if (child)r=child->processMouseEvent(event);
	/* Wenn next und child es noch NICHT verarbeitet hat, hier verarbeiten. */
	if (next && r==0)return next->processMouseEvent(event);
	/* Rueckgabe von child oder next zurueckgeben, sonst 0. */
	return r;
}


/**
 * Die WidgetPane benoetigt eine sehr eigene render Methode.
 * Sie existiert nur einmal und bereitet die 2D Engine vor, um alle anderen Widgets
 * ordentlich zu rendern.
 **/
void CWidgetPane::render(bool selectionBuffer)
{
	/* Wir stellen eine orthogonale Projektion ein. */
	glMatrixMode(GL_PROJECTION);
	if (!selectionBuffer)
	{
		glPushMatrix();
		glLoadIdentity();
	}else{
		glPopMatrix();
		glPushMatrix();
	}
	/* Die genau die Koordinaten (0/0) bis (640/480) bereitstellt und mit dem Fenster
	   skaliert. Z-Bereich geht von -640 bis +640, damit sich ein Fenster komplett
	   drehen kann. */
	glOrtho(0,640,480,0,-640,640);
	
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	/* Widgets werden ohne Z-Buffer und ohne Beleuchtung gerendert. */
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	/* Falls wir NICHT in den SelectionBuffer rendern, schalten wir Alpha-Blending ein */
	if (!selectionBuffer)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	}

	/* Materialien auf Standard setzen. */
	const float mat[]={1.0,1.0,1.0,1.0};
	const float spec[]={0,0,0,1};
	glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,mat);
	glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,spec);

	/* zuerst alle Widgets rendern */
	CWidget::render(selectionBuffer);
	/* dann die SubChilds der Reihe nach durchrendern. */
	CWidget::renderSubChild(selectionBuffer);

	/* Alpha Blending aus, und DepthBuffer und Beleuchtung wieder ein */
	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);

	glPopMatrix();

	/* ggf. vorher gerettete Projektions-Matrix wiederherstellen. */
	if (!selectionBuffer)
	{
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	}
}

/**
 * Mausereignis verarbeiten. Dies verhaelt sich an dieser Stelle leicht anders,
 * wie die anderen Widgets. Existiert ein SubChild, und eine Nachricht wird
 * nicht verarbeitet, geb -1 zurueck, als Zeichen, dass die GUI das Ereignis nicht weiter
 * bearbeiten darf. 
 **/
int CWidgetPane::processMouseEvent(TMouseEvent *event)
{
	/* Erstmal Nachricht verarbeiten. Default-Handler des CWidget aufrufen. */
	int r=CWidget::processMouseEvent(event);
	/* Wenn die Nachricht nicht bearbeitet wurde, aber subChilds existieren, 
	   dann -1 zurueck geben! */
	if (r==0 && hasSubChild())return -1;
	/* Sonst 0 */
	if (r==0 && !hasSubChild())return 0;
	return r;
}


/**
 * Constructs einen CButton, an Stelle (vx/vy), vw breit, vh hoch, mit id und Text.
 * Die Position wird relativ zu parent berechnet.
 **/
CButton::CButton(double vx,double vy,double vw,double vh,int id,CWidget* parent,const char *vtext)
{
	x=vx;
	y=vy;
	w=vw;
	h=vh;
	/* Der Button ist erstmal enabled */
	enabled=true;
	/* Ist parent!=NULL, relative Position zu parent in absolute umrechnen. */
	if (parent) {
		x+=parent->getX();
		y+=parent->getY();
	}
	setId(id);
	hovered=false;
	/* Animationen sind nicht gegenwaertig. */
	anim=pushanim=0.0;
	/* Beschriftungstext klonen. */
	text=strdup(vtext);
}

/**
 * Destruktor: Lediglich Speicher der Beschriftung wieder freigeben.
 **/
CButton::~CButton()
{
	if (text)free(text);
}

/**
 * Einen CButton rendern
 **/
void CButton::render(bool selectionBuffer)
{
	/* Nur wirklich was rendern, wenns nicht der SelectionBuffer ist.
	   das Rendern in den SelectionBuffer uebernimmt CWidget::render(true); */
	if (!selectionBuffer)
	{
		const double text_width=h*0.38;
		const double scale=(sin(anim*M_PI/2.0)*0.5+0.5)*0.2;

		double r,g,b,a;
		a=0.7+anim*0.3;
		r=0.1+anim*0.3+pushanim*0.2;
		g=0.2+anim*0.4+pushanim*0.3;
		b=0.4+anim*0.4+pushanim*0.2;

		/* Ich brauch ein Schriftobjekt */
		CGLFont font(text_width,h*0.9);
		glPushMatrix();
		glTranslated(x+w/2.0,y+h/2.0,0);
		glScaled(1.0+scale*0.2,1.0+scale*0.25,1);
		
		if (!enabled)a=0.3;
		/* Rechteck des Buttons rendern. */
		{
			glBegin(GL_QUADS);
			glColor4d(r,g,b,a);
			glVertex3d(-w/2.0,-h/2.0,0);
			glColor4d(r*0.5,g,b*0.8,a*0.7);
			glVertex3d(-w/2.0,h/2.0,0);
			glColor4d(r*0.5,g*0.7,b*0.7,a*0.7);
			glVertex3d(w/2.0,h/2.0,0);
			glColor4d(r*0.4,g*0.6,b*0.7,a*0.75);
			glVertex3d(w/2.0,-h/2.0,0);
			glEnd();
		}
		/* Bei Animation bei Gedrueckt rendere ein paar Rechtecke drum rum */
		if (pushanim>0.001)
		{
			glPushMatrix();
			glColor4d(0.9,0.95,1.0,pushanim*0.45);
			glScaled(1.0/(1.0+scale*0.5),1.0/(1.0+scale*0.25),1);
			for (int i=0;i<2;i++)
			{
				glScaled(1.1-pushanim*0.1,1.4-pushanim*0.4,1);
				glBegin(GL_LINE_LOOP);
				glVertex3d(-w/2.0,-h/2.0,0);
				glVertex3d(-w/2.0,h/2.0,0);
				glVertex3d(w/2.0,h/2.0,0);
				glVertex3d(w/2.0,-h/2.0,0);
				glEnd();
			}
			glPopMatrix();
		}
		/* Jetzt mal ordentlich Text rendern. Schattiert, nicht geschuettelt. */
		glScaled(1+scale*0.6,1+scale,1+scale);
		glColor4d(0.2,0.2,0.2,1.0);
		font.drawText((-text_width*strlen(text))/2.0+1,-h*0.45+1,text);
		if (enabled)glColor4d(1,1,1,1);
		else glColor4d(0.45,0.45,0.45,1);
		font.drawText((-text_width*strlen(text))/2.0,-h*0.45,text);
		/* Bei hover-Animation den Text leicht vergroessert nochmal rendern. */
		if (hovered && anim<0.999)
		{
			glScaled(1+scale*1.3,1+scale*2.9,1);
			glColor4d(0.8-anim*0.8,0.8-anim*0.8,0.8-anim*0.8,1);
			font.drawText((-text_width*strlen(text))/2.0,-h*0.45,text);
		}
		glPopMatrix();
	}
	/* Und CWidget::render kuemmert sich um die next und child Listen, sowie 
	   um das Rendern in den selectionBuffer. */
	CWidget::render(selectionBuffer);
}

/**
 * Button animieren, sofern eine Animation vorliegt (hover / press).
 **/
void CButton::execute(double elapsed)
{
	/* Wenn hovered, dann anim rauf animieren, bis 1. */
	if (hovered)
	{
		anim+=elapsed*3.9;
		if (anim>1.0)anim=1.0;
	}else{
		/* Sonst runter animieren, bis 0 */
		anim-=elapsed*1.4;
		if (anim<0.0)anim=0.0;
	}
	/* pushanim runter animieren, bis 0 */
	pushanim-=elapsed*2.5;
	if (pushanim<0.0)pushanim=0.0;
	/* Und an verkettete Listen uebergeben. */
	CWidget::execute(elapsed);
}

/**
 * Mausereignis des CButtons verarbeiten. Wenn der Button gedrueckt wurde, gebe id zurueck.
 **/
int CButton::processMouseEvent(TMouseEvent *event)
{
	/* Alle anderen Widgets haben Vorrang. Falls bearbeitet, Kommando nach unten durchreichen */
	int r=CWidget::processMouseEvent(event);
	if (r)return r;
	/* Und wenn kein next, child oder subchild die Nachricht verarbeitet hat,
	   und <hovered>... */
	if (!hovered)return 0;
	
	/* ...und die linke Maustaste gedrueckt wurde, sowie der Button eine Id hat */
	if (event->press && event->button1 && getId())
	{
		/* Dann wurde genau dieser Button betaetigt. pushanim starten. */
		pushanim=1.0;
		/* Und ID als Kommando zurueck geben */
		return getId();
	}
	/* Ansonsten 0 zurueckgeben, Nachricht wurde nicht bearbeitet. */
	return 0;
}


/**
 * Eine ChekBox
 * Position wurd durch vx,vy,vw,vh definiert
 * id und eine Beschriftung werden durch id und vtext uebergeben. vchecked gibt
 * vordefinierten Zustand an. parent wird nur zur benutzt, um vx/vy relativ angeben
 * zu koennen.
 **/
CCheckBox::CCheckBox(double vx,double vy,double vw,double vh,int id,const char *vtext,bool vchecked,CWidget* parent)
{
	/* Position merken. */
	x=vx;
	y=vy;
	w=vw;
	h=vh;
	/* Checkbox ist enabled. */
	enabled=true;
	/* Wenn parent uebergeben wurde, liegen vx und vy in relativen Koordinaten vor. 
	   Diese hier in absolute umrechnen. */
	if (parent) {
		x+=parent->getX();
		y+=parent->getY();
	}
	/* pushanim hat Endstadium erreicht. */
	pushanim=1.0;
	/* Id merken */
	setId(id);
	/* CheckBox ist vorerst nicht hovered. */
	hovered=false;
	/* Checked merken */
	checked=vchecked;
	/* Speicher fuer Beschriftungstext alloziieren und Text merken. */
	text=strdup(vtext);
	/* Die verketteten Listen fuer Gruppen auf leer setzen. */
	nextCheckBox=prevCheckBox=0;
}

/**
 * CheckBox aufraeumen, hier lediglich Speicher fuer Beschriftungstext freigeben.
 **/
CCheckBox::~CCheckBox()
{
	if (text)free(text);
}

/**
 * CCheckBox animieren.
 **/
void CCheckBox::execute(double elapsed)
{
	/* Rest der Listen animieren. */
	CWidget::execute(elapsed);
	/* pushanim von 0 bis 1 hoch animieren. */
	pushanim+=elapsed*2.0;
	if (pushanim>1.0)pushanim=1.0;
}

/**
 * CheckBox rendern.
 **/
void CCheckBox::render(bool selectionBuffer)
{
	/* Nur rendern, wenn nicht in den SelectionBuffer gerendert wird. Das Rendern in
	   eben diesen erledigt CWidget::render(true); */
	if (!selectionBuffer)
	{
		static const double text_width=8.0;
		CGLFont font(text_width,h*0.9);
		
		const double a=(enabled?1.0:0.4);
		/* Das Rechteck der "CheckBox" links rendern. */
		glBegin(GL_QUADS);
		if (hovered)
		{
			glColor4d(0.3,0.7,0.8,0.7);
			glVertex3d(x+h*0.05,y+h*0.05,0);
			glColor4d(0.2,0.35,0.45,0.5);
			glVertex3d(x+h*0.05,y+h*0.95,0);
			glColor4d(0.1,0.35,0.45,0.4);
			glVertex3d(x+h*0.95,y+h*0.95,0);
			glColor4d(0.2,0.6,0.7,0.5);
			glVertex3d(x+h*0.95,y+h*0.05,0);
		}else{
			glColor4d(0.1,0.2,0.3,a*0.7);
			glVertex3d(x+h*0.05,y+h*0.05,0);
			glColor4d(0.1,0.1,0.2,a*0.6);
			glVertex3d(x+h*0.05,y+h*0.95,0);
			glColor4d(0.1,0.1,0.2,a*0.5);
			glVertex3d(x+h*0.95,y+h*0.95,0);
			glColor4d(0.1,0.2,0.2,a*0.6);
			glVertex3d(x+h*0.95,y+h*0.05,0);
		}
		glEnd();

		if (checked || pushanim<1.0) {
			/* Das Haeckchen rendern, ggf. vorher drehen*/
			glPushMatrix();
			glTranslated(x+h*0.5,y+h*0.5,0);
			glPushMatrix();
			if (checked)glRotated(90.0-pushanim*90.0,0.2,0.9,0);
			else glRotated(pushanim*90.0,0.2,0.9,0);;
		
			glBegin(GL_TRIANGLE_STRIP);
			glColor4d(1,1,0.8,0.7*a);
			glVertex3d(-h*0.4,-h*0.2,0);
			glVertex3d(-h*0.1,h*0.4,0);
			glVertex3d(-h*0.1,0,0);
			glVertex3d(h*0.4,-h*0.4,0);
			glEnd();
			glPopMatrix();
			if (pushanim<1.0)
			{
				/* Bei der Animatioin das Haeckchen noch einmal groesser
				   rendern. */
				glScaled(1.0+pushanim*3.0,1.0+pushanim*3.0,1);
				if (checked)glRotated(-45.0+pushanim*45.0,0,0,-1);
				else glRotated(pushanim*45.0,0,0,-1);
				glBegin(GL_TRIANGLE_STRIP);
				if (checked)glColor4d(0.3,1,0.3,1.0-pushanim);
				else glColor4d(1.0,0.3,0.3,1.0-pushanim);
				glVertex3d(-h*0.4,-h*0.2,0);
				glVertex3d(-h*0.1,h*0.4,0);
				glVertex3d(-h*0.1,0,0);
				glVertex3d(h*0.4,-h*0.4,0);
				glEnd();
			}
			glPopMatrix();
		}
		glPushMatrix();
		/* Den Beschriftungstext rendern. */
		glTranslated(x+40.0+h,y+h/2.0,0);
		glColor4d(0.2,0.2,0.2,1);
		/* Erst Schatten. */
		font.drawText(1-30,-h*0.9/2.0+1,text);
		if (hovered)glColor4d(1,1,1,1);
		else if (enabled)glColor4d(0.65,0.65,0.65,1);
		else glColor4d(0.3,0.3,0.3,1);
		/* Dann echten Text drueber. */
		font.drawText(-30,-h*0.9/2.0,text);
		/* Wenn die Animation laeuft, den Text nochmal in groesser drueber. */
		if (pushanim<1.0)
		{
			if (!checked)glScaled(1.0+pushanim/3.0,1.0+pushanim*1.3,1);
			else glScaled(1.3-pushanim/3.0,2.3-pushanim*1.3,1);
			glColor4d(1.0-pushanim,1.0-pushanim,1.0-pushanim,1);
			font.drawText(-30,-h*0.9/2.0,text);
		}
		glPopMatrix();
	}
	/* Restlichen Widgets rendern. */
	CWidget::render(selectionBuffer);
}

/**
 * Mausereignis verarbeiten, sofern es kein anderer tut.
 * Wenn die CheckBox gecheckt wird, automatisch alle anderen CheckBoxes derselben Gruppe 
 * entchecken. :)
 **/
int CCheckBox::processMouseEvent(TMouseEvent *event)
{
	/* Alle anderen Widgets haben erstmal Vorrang. */
	int r=CWidget::processMouseEvent(event);
	/* Wird das Ereignis bearbeitet, raus hier. */
	if (r)return r;
	/* Wenn die CheckBox nicht ge-hovert ist, ist das Ereignis auch nicht fuer sie */
	if (!hovered)return 0;

	/* Bei Druecken der linken Maustaste, klickt also jemand auf die Checkbox. */
	if (event->press && event->button1)
	{
		/* Nur un-checken erlauben, wenn die Checkbox allein in der Gruppe ist,
		   dann ist sie eine echte CheckBox (an/aus). */
		if (checked && !nextCheckBox && !prevCheckBox)
		{
			/* Animieren, und getId() zurueckgeben. */
			pushanim=0.0;
			checked=false;
			return getId();
		}else 
		/* Nur weiter machen, wenn die CheckBox aus ist. */
			if (!checked)
		{
			/* Dann mach sie an, und animiere sie. */
			checked=true;
			pushanim=0.0;
			/* Sofern die CheckBox zu einer Gruppe gehoert, hier alle Elemente der
			   nextCheckBox-Liste und prevCheckBox-Liste auf unchecked setzen.
			   Damit haben wir eine Radiogroup. */
			CCheckBox *b=nextCheckBox;
			/* Verkettete Listen iterativ durchgehen. */
			while (b)
			{
				b->setCheck(false);
				b=b->nextCheckBox;
			}
			b=prevCheckBox;
			while (b)
			{
				b->setCheck(false);
				b=b->prevCheckBox;
			}
			/* Und ID zurueck geben. */
			return getId();
		}
	}
	/* Event wurde nicht beachtet. 0 zurueck geben. */
	return 0;
}

/**
 * Eine CheckBox der Gruppe von CheckBoxes anfuegen.
 **/
void CCheckBox::addCheckBox(CCheckBox *b)
{
	/* Einfach rekursiv der Liste hinten anhaengen. */
	if (nextCheckBox)nextCheckBox->addCheckBox(b);
	else
	{
		/* Schoen verketten. Die aktuelle CheckBox muss vorwaerts durch nextCheckBox 
		   kommen, und die andere muss rueckwaerts gehen koennen, durch 
		   prevCheckBox. */
		nextCheckBox=b;
		b->prevCheckBox=this;
	}
}



/**
 * Ein CStaticText ist nur ein statischer Text
 * Position ist (x/y), ggf. relativ zu parent, mit Beschriftung vtext.
 * In vtext koennen \n auftreten, als Zeilenumbruch. Der Text wird links-ausgerichtet.
 **/
CStaticText::CStaticText(double x,double y,const char *vtext,CWidget* parent)
:CWidget()
{
	/* Alpha-Wert ist 1. Also voll opak. */
	a=1.0;
	/* Position merken. */
	this->x=x;
	this->y=y;
	/* Ein Zeichen ist 8 Einheiten breit, also hier Breite des Widgets errechnen, damit der
	   Text links an x ausgerichtet wird. */
	this->w=8*strlen(vtext);
	/* Wenn Koordinaten relativ zu parent angegeben wurden, hier auf absolute umrechnen. */
	if (parent)
	{
		this->x+=parent->getX();
		this->y+=parent->getY();
	}
	/* Text kopieren. */
	text=strdup(vtext);
}

/**
 * Konstruktor wie oben, nur dass hier ein Parameter w uebergeben wird,
 * der die Breite des Texts angibt. Der Text wird innerhalb von w mittit ausgerichtet.
 **/
CStaticText::CStaticText(double x,double y,double w,const char *vtext,CWidget* parent)
:CWidget()
{
	a=1.0;
	this->x=x;
	this->y=y;
	this->w=w;
	if (parent)
	{
		this->x+=parent->getX();
		this->y+=parent->getY();
	}
	text=strdup(vtext);
}

/**
 * Destructor. Speicher fuer text wieder freigeben.
 **/
CStaticText::~CStaticText()
{	
	if (text)free(text);
}

/**
 * Einen neuen Text setzen. 
 * Bei adjustwidth=false, bleibe <w> erhalten, der neue Text also innerhalb der alten Breite
 * in der Mitte ausgerichtet. */
void CStaticText::setText(const char *vtext,bool adjustwidth)
{
	/* Alten Speicher freigeben. */
	if (text)free(text);
	/* und neuen alloziieren und text kopieren. */
	text=strdup(vtext);
	/* Neue Breite des Widgets an neue Textlaenge anpassen? */
	if (adjustwidth) w=8*strlen(vtext);
	/* Etwas Transparenz setzen */
	setAlpha(0.625);
}

/**
 * StaticText rendern. */
void CStaticText::render(bool selectionbuffer)
{
	/* Wenn nicht in den SelectionBuffer gerendert werden soll. In den SelectionBuffer wird
	   eh nix gerendert, da wahrscheinlich (getId()==0) */
	if (!selectionbuffer && !deleted())
	{
		/* Ein Schriftobjekt fester Groesse holen, und Text rendern. */
		CGLFont font(8,15);
		/* Transparenz-Grad setzen. */
		glColor4d(a,a,a,1.0);
		/* Text zentriert an x mit Breite w ausrichten und rendern. */
		font.drawText(x+(w-8*strlen(text))/2.0,y,text);
	}
	/* Andere Widgets rendern. */
	CWidget::render(selectionbuffer);
}



/**
 * Ein Text-Feld. Posistion durch x,y,w,h definiert, ggf. relativ zu parent..
 * id ist id und vtext der voreingestellte Text.
 * Ist numbers_only=true, koennen nur Zahlen eingegeben werden, sonst auch Buchstaben. 
 **/
CTextEdit::CTextEdit(double x,double y,double w,double h,int id,const char *vtext,bool numbers_only,CWidget *parent)
/* Wir haben uns ja von CFrame abgeleitet, um einen normalen Hintergrund zu erhalten. */
:CFrame(x,y,w,h)
{
	/* Koordinaten umrechnen. */
	if (parent) {
		this->x+=parent->getX();
		this->y+=parent->getY();
	}
	setId(id);
	/* text ist ein festes char-Array, also nur vtext da rein kopieren. Keinen Speicher alloziieren. */
	setText(vtext);
	numbers=numbers_only;
	animatefocused=0.0;
	animatechar=1.0;
	hovered=focused=false;
	m_committed=false;
	/* Der Text in dem Edit soll links ausgerichtet sein. Koennte man auch auf false setzen,
	   fuer Rechtsbuendig. So machts die CSpinBox. */
	left_align=true;
	/* CFrame sagen, dass wir 0.55 opacity haben wollen. */
	setAlpha(0.55);
}

/**
 * Destructor. Wir haben (noch) nix zum aufraeumen.
 **/
CTextEdit::~CTextEdit()
{ }

/** Zeiger auf text zurueckgeben. */
const char* CTextEdit::getText()
{
	return &text[0];
}

/**
 * Neuen Text setzen.
 * m_committet zeigt an, ob der Text mit <Enter> bestaetigt wurde. Das muss hier zurueck gesetzt
 * werden.
 **/
void CTextEdit::setText(const char *vtext)
{
	m_committed=false;
	/* Einfach Text kopieren, keinen Speicher alloziieren, da text ein Array fester Groesse ist. */
	strncpy(text,vtext,sizeof(text));
}

/**
 * Das TextEdit rendern. Einen Teil rendert CFrame::render()    o_O
 **/
void CTextEdit::render(bool selection)
{
	/* Hier jedesmal die Farben vom CFrame den Gegebenheiten anpassen. Kostet ja nix und ist fix. */
	if (focused)setColor(0.4,0.5,0.65);
	else if (hovered)setColor(0.25,0.3,0.45);
	else setColor(0.1,0.15,0.2);
	/* CFrame rendern. Dies rendert erstmal ein farbiges Quadrat sowie alle anderen Widgets 
	   derselben Ebene, sowie das Widget in den SelectionBuffer, falls selection=true. */
	CFrame::render(selection);

	/* Wenn nicht in den selectionBuffer gerendert wird, malen wir hier froehlich weiter. */
	if (!selection)
	{
		
		double tw=h*0.4;
		double otw=tw;
		/* Wenn die Breite des Texts die des Widgets ueberschreitet, berechne Buchstabenbreite
		   unter Einbeziehung der Animation dynamisch neu. Muss man nicht verstehen.
		   (raff ich selbst nicht mehr. :) */
		if ((strlen(text)+2)*tw>w)tw=w/(double)((double)strlen(text)+1.0+(lastchar?(1.0-animatechar):animatechar));
		CGLFont font(tw,h*0.90);
	
		/* Hier ein huebsches weisses Rechteck um das Frame ziehen. */
		glColor4d(1,1,1,hovered?0.8:0.4);
		glBegin(GL_LINE_LOOP);
		glVertex3d(x+w,y,0);
		glVertex3d(x,y,0);
		glVertex3d(x,y+h,0);
		glVertex3d(x+w,y+h,0);
		glEnd();
	
		/* Wenn das Edit-Feld den Fokus hat, seicht animierte weitere Rechtecke um das Feld ziehen. */
		if (focused) {
			glPushMatrix();
			glTranslated(x+w/2.0,y+h/2.0,0);
			/* derer zwo */
			for (int i=0;i<2;i++)
			{
				/* das zweite genau um eine halbe Periode versetzt. */
				const double anim=fmod(animatefocused+(double)i/2.0,1);
				glPushMatrix();
				glColor4d(1,1,1,0.3-anim*0.3);
				glScaled(1.0+(anim)*0.10,1.0+(anim)*1.0,1);
				glBegin(GL_LINE_LOOP);
				glVertex3d(+w/2.0,-h/2.0,0);
				glVertex3d(-w/2.0,-h/2.0,0);
				glVertex3d(-w/2.0,+h/2.0,0);
				glVertex3d(+w/2.0,+h/2.0,0);
				glEnd();
				glPopMatrix();
			}

			glPopMatrix();
		}
	
		glPushMatrix();
		glTranslated(x+4,y+h*0.05,0);
		glColor4d(1,1,1,1);
		/* Wenn ein Zeichen entfernt wurde, oder ein neues getippt, so wird das letzte Zeichen
		   hier animiert dargestellt. */
		if (animatechar<1.0)
		{
			/* lastchar=='\0', dann wurde ein neues Zeichen getippt. Das zu animierende
			   ist das letzte von text[]. */
			if (lastchar=='\0')
			{
				/* Zuerst alle Zeichen rendern, bis auf das Letzte. */
				if (strlen(text)>1)font.drawText(left_align?0:(w-tw*strlen(text)-otw),0,text,strlen(text)-1);
				/* Und das Letzte wird hier separat animiert dargestellt. */
				if (strlen(text)>=1)
				{
					const double s=5.0-animatechar*4.0;
					glColor4d(0.5+animatechar*0.5,0.5+animatechar*0.5,0.5+animatechar*0.5,1);
					if (left_align)glTranslated((strlen(text)-1)*tw+tw/2.0,h*0.45,0);
					else glTranslated(w-otw*1.5,h*0.45,0);
					glScaled(s,s,s);
					font.drawText(-tw/2.0,-h*0.45,&text[strlen(text)-1],1);
				}
			}else{
				/* Sonst wurde das letzte Zeichen entfernt, welches in lastchar steht. 
				   Dies soll hier animiert dargestellt werden. */
				font.drawText(left_align?0:(w-tw*strlen(text)-otw),0,text);
				const double s=1.0+animatechar*4.0;
				glColor4d(1.0-animatechar,1.0-animatechar,1.0-animatechar,1);
				if (left_align)glTranslated((strlen(text))*tw+tw/2.0,h*0.45,0);
				else glTranslated(w-otw/2.0,h*0.45,0);
				glScaled(s,s,s);
				font.drawText(-tw/2.0,-h*0.45,&lastchar,1);
			}
		}else 
		/* wenn keine Animation vorliegt, kann der Text am Stueck gerendert werden. */
			font.drawText(left_align?0:(w-tw*strlen(text)-otw),0,text);
		glPopMatrix();
	}
}

/**
 * TextEdit animieren.
 **/
void CTextEdit::execute(double elapsed)
{
	/* Parent animieren. Kuemmert sich um alle anderen Widgets. */
	CFrame::execute(elapsed);
	/* Wenn das Feld nen Fokus hat, diesen animieren. */
	if (focused){
		animatefocused+=elapsed*0.6;
	}
	/* Wenn ein Zeichen animiert wird, animieren. */
	animatechar+=elapsed*8.0;
	if (animatechar>1.0)animatechar=1.0;
}

/**
 * Mausereignis verarbeiten.
 **/
int CTextEdit::processMouseEvent(TMouseEvent *event)
{
	/* Wenn das Ereignis von anderen Widgets verarbeitet wird, hier raus. */
	int r=CWidget::processMouseEvent(event);
	if (r)return r;

	/* Wenn linke Maustaste gedrueckt. */
	if (event->press && event->button1)
	{
		if (!focused)animatefocused=0.0;
		/* Wenn die Maus nicht ueber Textfeld ist, den Fokus nehmen. */
		if (!hovered)
		{
			/* Wenn das Feld vorher den Fokus hatte, bisherige Eingabe validieren. */
			if (focused)endEdit();
			focused=false; 
		}else focused=true;	// Das Feld ist hovered, und die Maus gedrueckt, so gib dem Feld den Fokus
	}
	/* Die rechte Maustaste wurde gedrueckt. */
	if (event->press && event->button2)
	{
		/* Wenn die Maus ueber dem Textfeld ist. */
		if (hovered)
		{
			/* Textfeld leeren! */
			setText("");
			/* (Eingabe validieren) */
			endEdit();
			animatefocused=0.0;
			/* Fokus geben, da wir auf dem Feld sind. */
			focused=true;
			return 0;
		}else{
			/* Eingabe validieren, wenn wir vorher den Fokus hatten. */
			if (focused)endEdit();
			/* Sonst einfach Fokus nehmen, da wir woanders hin gedrueckt haben. */
			focused=false;
		}
	}
	/* Behandlung des Ereignisses darf fortgesetzt werden. */
	return 0;
}

/**
 * Tastendruck verarbeiten
 **/
bool CTextEdit::processKey(unsigned short key)
{
	/* Wenn jemand anders schneller war, raus. */
	if (CFrame::processKey(key))return true;
	/* Wenn wir nicht den Fokus haben, brauchen wir nix zu behandeln. */
	if (!focused)return false;
	
	/* Pruefen, ob Taste ein Eingabezeichen ist. Umlaute sind auch erlaubt. */
	if ((   (isascii(key) && isprint(key))   || (strchr("äöüÄÖÜß",key)))
	   && ((!numbers) || (numbers && isdigit(key))))
	{
		/* Fruehzeitig keine neuen Zeichen mehr erlauben. Unser Eingabepuffer ist fest. */
		if (strlen(text)>sizeof(text)-2)return false;
		/* Buchstaben hinten anfuegen und auf terminierende \0 achten (besonders bei strlen) */
		text[strlen(text)+1]='\0';
		text[strlen(text)]=(unsigned char)(key&0xFF);
		/* das Zeichen animieren. */
		animatechar=0.0;
		/* Und anzeigen, dass das animierte Zeichen das letzte aus text[] ist. */
		lastchar='\0';
	}else{
		/* Es ist ein Steuerzeichen. */

		/* Es ist ein Backspace. Wenn wir einen text haben, das letzte Zeichen loeschen. */
		if (key==VK_BACK && strlen(text)>0)
		{
			/* Das letzte Zeichen merken, da dies noch animiert werden soll. */
			lastchar=text[strlen(text)-1];
			animatechar=0.0;
			/* Und einfach ne abschliessende terminierende \0 ueber das letzte Zeichen legen. */
			text[strlen(text)-1]='\0';
		}
		/* Escape. Das Textfeld verliert nur den Fokus. */
		if (key==VK_ESCAPE)
		{
			/* Eingabe validieren. */
			endEdit();
			focused=false;
		}
		/* Enter. Das Textfeld verliert den Fokus, aber m_committed wird true. */
		if (key==VK_RETURN)
		{
			/* Eingabe validieren. */
			endEdit();
			focused=false;
			/* Aber merken, dass Enter gedrueckt wurde, sodass noch allerhand getrieben werden kann. */
			m_committed=true;
		}
	}
	/* Wir wollen nicht, dass andere Widgets die Taste noch weiter verarbeiten, da wir den Fokus haben. */
	return true;
}


/**
 * Eine CSpinBox ist ein CTextEdit, erweitert um zwei Knoeppe und die Bereiche min und max. 
 * min, max, value sind die Bereiche, bzw. der aktuelle Zahlenwert der SpinBox.
 **/
CSpinBox::CSpinBox(double x,double y,double w,double h,int id,int min,int max,int value,CWidget *parent,bool rightButtons)
/* Parameter an CTextEdit uebergeben. */
:CTextEdit(x,y,w-21,h,id,"",true,parent)
{
	/* Wir wollen ein rechts-buendiges TextEdit. */
	left_align=false; 
	/* Grenzen merken. */
	this->min=min;
	this->max=max;
	this->def=value;
	/* Dem TextEdit Vorfahren zwei child Widgets anhaengen, die Knoepfe hoch und runter.
	   als ID kriegen sie 10000000+id fuer hoch, und 20000000+id fuer runter. */
	if (rightButtons){
		addChild(up=new CButton(w-19,-h/10.0,18,h/1.9,10000000+id,this,"^"));
		addChild(down=new CButton(w-19,h-h/2.0+h/10.0,18,h/1.9,20000000+id,this,"v"));
	}else{
		setSize(x+21,y,w-21,h);
		if (parent) {
			this->x+=parent->getX();
			this->y+=parent->getY();
		}
		addChild(up=new CButton(-20,-h/10.0,18,h/1.9,10000000+id,this,"^"));
		addChild(down=new CButton(-20,h-h/2.0+h/10.0,18,h/1.9,20000000+id,this,"v"));
	}
	/* Und den Anfangswert setzen. */
	setValue(value);
}

/**
 * Gibt den momentan eingegebenen Wert der SpinBox als Zahl zurueck.
 **/
int CSpinBox::getValue()
{
	return atoi(text);
}

/**
 * Setze den uebergebenen Wert als Text in das CTextEdit.
 * value wird automatisch in den Bereich min<=value<=max eingepasst, und die Knoepfe fuer hoch/runter
 * automatisch entsprechend de-/aktiviert.
 **/
void CSpinBox::setValue(int value)
{
	/* value anpassen, falls es aus dem erlaubten Bereich faellt */
	if (value<min)value=min;
	if (value>max)value=max;
	/* value als Zeichenkette in text aus dem TextEdit-Feld schreiben. */
	sprintf(text,"%d",value);

	/* up ist enabled, wenn value<max, also man den Wert noch erhoehen koennte. */
	up->setEnabled(value<max);
	/* Andersrum bei down. */
	down->setEnabled(value>min);
}

/**
 * Mausereignis verarbeiten.
 **/
int CSpinBox::processMouseEvent(TMouseEvent *event)
{
	/* Uebergeordnetes Widget die Taste verarbeiten lassen. */
	int r=CTextEdit::processMouseEvent(event);
	/* Wenn 10000000+getId() (dann wurde up betaetigt), oder das Mausrad hoch bewegt wurde,
	   Wert um 1 erhoehen. */
	if (r==10000000+getId() || (hovered && event->press && event->button4))setValue(getValue()+1);
	/* Wenn 20000000+getId() (dann wurde up betaetigt), oder das Mausrad runter bewegt wurde,
	   Wert um 1 erniedrigen. */
	else if (r==20000000+getId()  || (hovered && event->press && event->button5))setValue(getValue()-1);
	/* Sonst Rueckgabe von CTextEdit weiter nach oben durchreichen. */
	else return r;
	/* Ansonsten wurde der Wert veraendert. :) Also getId() zurueckgeben. */
	return getId();
}

/**
 * Als Validierung einfach den aktuell als Text vorhandenen Text mit setValue setzen.
 * setValue uebernimmt die Validierung und passt den Wert den Grenzen an.
 **/
void CSpinBox::endEdit()
{
	if (strlen(text)==0)setValue(def);
	else setValue(getValue());
}



/**
 * Ein Frame ist nur ein rechteckiges Feld auf dem Bildschirm, mit Farbe und Transparenz, aber ohne
 * Interaktivitaet.
 **/
CFrame::CFrame(double vx,double vy,double vw,double vh)
{
	/* Frame Position merken. */
	x=vx;
	y=vy;
	w=vw;
	h=vh;
	/* Und auf Standardfarbe / Standardtransparenz setzen. */
	setColor(0.8,0.8,0.8);
	setAlpha(0.92);
}

void CFrame::setSize(double vx,double vy,double vw,double vh)
{
	x=vx;
	y=vy;
	w=vw;
	h=vh;
}

void CFrame::render(bool selectionBuffer)
{
	if (!selectionBuffer)
	{
		/* Nicht in den SelectionBuffer rendern, aber sonst ein Rechteck der ungefaehren 
		   angegebenen Farbe, und Transparenz. */
		glBegin(GL_QUADS);
		glColor4d(r*0.75,g*0.75,b*0.75,a*0.9);
		glVertex3d(x+w,y,0);
		glColor4d(r,g,b,a);
		glVertex3d(x,y,0);
		glColor4d(r*0.75,g*0.75,b*0.75,a*0.9);
		glVertex3d(x,y+h,0);
		glColor4d(r*0.5,g*0.5,b*0.5,a*0.8);
		glVertex3d(x+w,y+h,0);
		glEnd();
	}
	/* Andere Widgets rendern. */
	CWidget::render(selectionBuffer);
}




/**
 * CDialog errichten. 
 **/
CDialog::CDialog(double vw,double vh,const char *vcaption)
/* Dialog ist ein zentrierter CFrame mit bestimmter Groesse. */
:CFrame((640.0-vw)/2.0,(480.0-vh)/2.0,vw,vh)
{
	/* Animation ist am Anfang. */
	anim=0.0;
	starting=true;
	/* Titelleistentext kopieren. */
	caption=strdup(vcaption);
	/* Standardfarbe setzen. */
	setColor(0.6,0.6,0.6);
}

/* Titelleistentext wieder freigeben. */
CDialog::~CDialog()
{
	if (caption)free(caption);
}

void CDialog::setSize(double vw,double vh)
{
	CFrame::setSize((640.0-vw)/2.0,(480.0-vh)/2.0,vw,vh);
}

void CDialog::setCaption(const char* caption)
{
	if (this->caption)free(this->caption);
	this->caption=strdup(caption);
}

/**
 * CDialog rendern. Das besteht hauptsaechlich aus einer Animation, den Rest macht das CFrame. 
 **/
void CDialog::render(bool selectionBuffer)
{
	glPushMatrix();
	glLoadIdentity();
	/* Dialog entsprechend der Animation drehen/skalieren um ihn schick einzublenden. */
	glRotated(-90.0+sin(anim*M_PI/2.0)*90.0,0.63,0.63,0.2);
	glScaled(0.9+sin(anim*M_PI/2.0)*0.1,1,1);
	glTranslated(320,240,0);
	glRotated(-30.0+sin(anim*M_PI/2.0)*30.0,0.1,0,-1);
	glTranslated(-320,-240,0);

	if (!selectionBuffer)
	{
		/* Einen transparenten, etwas versetzten, Schatten rendern. */
		const double shadowsize=9.0;
		glBegin(GL_QUADS);
		glColor4d(0,0,0,0.35);
		glVertex3d(x+w+shadowsize/1.5,y+shadowsize,0);
		glVertex3d(x+shadowsize/1.5,y+shadowsize,0);
		glVertex3d(x+shadowsize/1.5,y+h+shadowsize,0);
		glVertex3d(x+w+shadowsize/1.5,y+h+shadowsize,0);
		glEnd();
	}
	/* Dann das CFrame rendern, welches das Rechteck malt, und alle Widgets. */
	CFrame::render(selectionBuffer);
	
	/* Danach den Titelleistentext zentriert rendern. */
	CGLFont font(10,20);
	glColor4d(1,1,1,1);
	font.drawText(x+(w-10*strlen(caption))/2.0,y+5,caption);

	glPopMatrix();
}

/**
 * Dialog animieren.
 **/
void CDialog::execute(double elapsed)
{
	CWidget::execute(elapsed);
	const double speed1=1.3;
	const double speed2=1.9;

	if (starting)
	{
		/* Dialog einblenden. Animation vorwaerts. */
		anim+=elapsed*speed1;
		if (anim>1.0)anim=1.0;
	}else{
		/* Dialog ausblenden. Animation rueckwaerts. */
		anim-=elapsed*speed2;
		/* Und wenn die Animation vorbei ist, Dialog zum Entfernen markieren. */
		if (anim<0.0)
		{
			anim=0.0;
			remove();
		}
	}
}

/**
 * Mausereignis / Kommandos verarbeiten. 
 **/
int CDialog::processMouseEvent(TMouseEvent *event)
{
	if (!starting)return 0;
	int r=CWidget::processMouseEvent(event);
	/* Wenn als Kommando 1 zurueckgegeben wurde... */
	switch (r)
	{
	// Dialog schliessen. Das ist einfach mal als Close-Button definiert, den es ja fast ueberall gibt. 
	case 1:	close();
		break;
	default: return r;
	}
	// 0 zurueck, da wir den Knopf behandelt haben. 
	return 0;
}

/**
 * Taste verarbeiten.
 **/
bool CDialog::processKey(unsigned short key)
{
	/* Wenn schon bearbeitet, geh raus. */
	if (CFrame::processKey(key))return true;
	
	/* Wenn der Dialog gerade geschlossen wird, nix bearbeiten. */
	if (starting==false)return false;

	/* Wenn Escape gedrueckt, Dialog schliessen. */
	if (key==VK_ESCAPE){
		close();
		return true;
	}
	/* Ein Dialog verarbeitet IMMER die Taste, egal ob modal oder nicht. 
	   Kein anderer soll sie nach uns noch verarbeiten. */
	return true;
}

/* Dialog schliessen, samt aller subChilds. */
void CDialog::close()
{
	close(true);
}

/* Dialog schliessen. Rekursiv, oder nur der eine Dialog? */
void CDialog::close(bool recursive)
{
	/* Wenn rekursiv, werden Unterdialoge mit geschlossen, sonst nur der aktuelle Dialog. */
	if (recursive && hasSubChild())getSubChild()->close();
	/* Schliess-Animation starten. */
	starting=false;
}

/**
 * Nur eine MessageBox, mit Titelleiste, Text und Farbe des Fensters in (r/g/b)
 **/
CMessageBox::CMessageBox(double w,double h,const char *caption,const char *text,double r,double g,double b)
:CDialog(w,h,caption)
{
	/* Farbe setzen. */
	setColor(r,g,b);
	/* Ok-Knopf einfuegen. */
	addChild(new CButton((w-70)/2.0,h-30,70,25,1,this,"Okay"));
	/* Text als StaticText ins Fenster plazieren. */
	addChild(new CStaticText(10,40,text,this));
}





/**
 * Ein FPS Zaehler als CStaticText. Zaehler in Konstruktor auf 0 setzen
 **/
CFPS::CFPS(double x,double y,CGUI *gui)
:CStaticText(x,y,"",NULL)
{
	counter=0;
	time=0.0;
	/* Optionen der GUI merken. */
	options=gui->getOptions();
}

/**
 * Prueft, ob die Zeit fuer eine Messung abgelaufen ist, und passt den Schriftzug an.
 **/
void CFPS::execute(double elapsed)
{
	CStaticText::execute(elapsed);
	time+=elapsed;
	if (time>0.5)
	{
		/* Schriftzug anpassen. */
		char c[100];
		sprintf(c,"fps: %.1f",(double)counter / time);
		setText(c,true);
		/* Zaehler zuruecksetzen. */
		time=0.0;
		counter=0;
	}
}

/**
 * Text rendern, und Zaehler fuer Frames erhoehen.
 **/
void CFPS::render(bool selectionBuffer)
{
	/* Wenn der Text gerendert werden soll, ruf CStaticText::render auf. Dieser ruft CWidget::render 
	   auf. Ansonsten ruf CWidget::render direkt auf, und umgeh CStaticText. */
	if (options->get(OPTION_SHOW_FPS))CStaticText::render(selectionBuffer);
	else CWidget::render(selectionBuffer);

	counter++;
}

