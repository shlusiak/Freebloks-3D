/**
 * options.cpp
 * Autor: Sascha Hlusiak
 * Dialog zum Einstellen der Optionen.
 **/

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "options.h"
#include "widgets.h"
#include "gui.h"



class CFPSSpinBox:public CSpinBox
{
private:
	TOptions *opt;
	virtual void endEdit() {
		CSpinBox::endEdit();
		opt->set(OPTION_FPS,getValue());
	}
public:
	CFPSSpinBox(double x,double y,double w,double h,int id,int min,int max,CWidget *parent,TOptions* vopt)
	:CSpinBox(x,y,w,h,id,min,max,35,parent)
	{
		setValue(vopt->get(OPTION_FPS));
		opt=vopt;
	}

};


/**
 * OptionsDialog erstellen, Widgets hinzufuegen und Wert aus den Optionen zuteilen.
 **/
COptionsDialog::COptionsDialog(CGUI *vgui)
:CDialog(400,310,"Options")
{
	/* Farbe setzen. */
	setColor(0.35,0.50,0.85);
	/* Aus der GUI die TOptions holen, als Zeiger. Wir koennen diese direkt veraendern. */
	options=vgui->getOptions();
	fps=nullptr;

	/* CheckBoxes hinzufuegen. */
	addChild(shadow=new CCheckBox(20,50,160,20,10000,"Render Shadows",options->get(OPTION_SHADOW)!=0,this));
	addChild(env=new CCheckBox(20,75,290,20,10004,"Render textured Environment",options->get(OPTION_ENVIRONMENT)!=0,this));
	addChild(hints=new CCheckBox(20,100,290,20,10006,"Mark possible corners on field",options->get(OPTION_SHOW_HINTS)!=0,this));
	addChild(limitfps=new CCheckBox(20,125,110,20,10007,"Limit FPS",options->get(OPTION_LIMITFPS)!=0,this));
	addChild(showfps=new CCheckBox(20,150,290,20,10005,"Show FPS",options->get(OPTION_SHOW_FPS)!=0,this));

	addChild(animatestones=new CCheckBox(20,180,160,20,10001,"Animate Stones",options->get(OPTION_ANIMATE_STONES)!=0,this));
	addChild(animatearrows=new CCheckBox(20,205,300,20,10003,"Animate Arrows of Selected Stone",options->get(OPTION_ANIMATE_ARROWS)!=0,this));
	addChild(autochat=new CCheckBox(20,230,240,20,10002,"Autoopen/Autoclose Chatbox",options->get(OPTION_AUTO_CHATBOX)!=0,this));

	/* Und Schliessknopf mit ID=1 nicht vergessen. */
	addChild(new CButton(160,270,80,30,1,this,"Okay"));

	if (options->get(OPTION_LIMITFPS))
		addChild(fps=new CFPSSpinBox(140,125,45,20,10008,2,99,this,options));
}

/**
 * Mausereignis verarbeiten, effektiv Aenderungen der Checkboxes.
 **/
int COptionsDialog::processMouseEvent(TMouseEvent *event)
{
	/* Dialog das Ereignis verarbeiten lassen. */
	int r=CDialog::processMouseEvent(event);
	switch (r)
	{
	/* Und gucken, ob das Ereignis von einer CheckBox verarbeitet wurde. Dann Option entsprechend setzen. */
	case 10000: options->set(OPTION_SHADOW,shadow->getCheck());
		break;
	case 10001: options->set(OPTION_ANIMATE_STONES,animatestones->getCheck());
		break;
	case 10002: options->set(OPTION_AUTO_CHATBOX,autochat->getCheck());
		break;
	case 10003: options->set(OPTION_ANIMATE_ARROWS,animatearrows->getCheck());
		break;
	case 10004: options->set(OPTION_ENVIRONMENT,env->getCheck());
		break;
	case 10005: options->set(OPTION_SHOW_FPS,showfps->getCheck());
		break;
	case 10006: options->set(OPTION_SHOW_HINTS,hints->getCheck());
		break;
	case 10007: options->set(OPTION_LIMITFPS,limitfps->getCheck());
		if (limitfps->getCheck() && !fps)
			addChild(fps=new CFPSSpinBox(140,125,45,20,10008,2,99,this,options));
		else if (fps)
		{
// 			options->set(OPTION_FPS,fps->getValue());
			fps->remove();
			fps=nullptr;
		}
		break;
	case 10008: if (fps)options->set(OPTION_FPS,fps->getValue());
		break;

	default: return r;
	}
	/* Bei Bearbeitung, nicht weiter verarbeiten. */
	return 0;
}


