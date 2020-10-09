/*
 * IpsDisplay.cpp
 *
 *  Created on: Oct 7, 2019
 *      Author: iltis
 *
 */


#include "sdkconfig.h"
#include <stdio.h>
#include <string.h>
//#include <Ucglib.h>
#include "IpsDisplay.h"
#include "BTSender.h"
#include "DallasRmt.h"
#include "freertos/task.h"
#include <logdef.h>


int   IpsDisplay::tick = 0;
bool  IpsDisplay::_menu = false;
int   IpsDisplay::_pixpmd = 10;
int   IpsDisplay::charge = 100;
int   IpsDisplay::red = 10;
int   IpsDisplay::yellow = 25;


#define DISPLAY_H 320
#define DISPLAY_W 240


// u8g2_t IpsDisplay::u8g2c;

const int   dmid = 160;   // display middle
const int   bwide = 64;   // total width of bargraph
const int   smfh  = 12;   // small font heigth
const int   hbw   = 12;   // horizontal bar width for unit of bargraph
const int   bw    = 32;   // bar width
const int   S2F_TRISIZE = 60; // triangle size quality up/down

#define DISPLAY_LEFT 25

#define TRISIZE 15
#define abs(x)  (x < 0.0 ? -x : x)

#define FIELD_START 85
#define SIGNLEN 24+4
#define GAP 12

#define HEADFONTH 16
#define VARFONTH  42  // fub35_hn
#define YVAR HEADFONTH+VARFONTH
#define YVARMID (YVAR - (VARFONTH/2))

#define S2FFONTH 31
#define YS2F YVAR+S2FFONTH+HEADFONTH+GAP-8


#define VARBARGAP (HEADFONTH+(HEADFONTH/2)+2)
#define MAXS2FTRI 43
#define MAXTEBAR ((DISPLAY_H-(VARBARGAP*2))/2)

#define YALT (YS2F+S2FFONTH+HEADFONTH+GAP+2*MAXS2FTRI +25 )

#define LOWBAT  11.6    // 20%  -> 0%
#define FULLBAT 12.8    // 100%

#define BTSIZE  5
#define BTW    15
#define BTH    24
#define ASVALX 165

int S2FST = 45;

#define UNITVAR (vario_unit.get())
#define UNITAS (ias_unit.get())
#define UNITALT (alt_unit.get())


int ASLEN = 0;
static int fh;

extern xSemaphoreHandle spiMutex;

#define PMLEN 24

ucg_color_t IpsDisplay::colors[TEMAX+1+TEGAP];
ucg_color_t IpsDisplay::colorsalt[TEMAX+1+TEGAP];


Ucglib_ILI9341_18x240x320_HWSPI *IpsDisplay::ucg = 0;

int IpsDisplay::_te=0;
int IpsDisplay::_ate=0;
int IpsDisplay::s2falt=-1;
int IpsDisplay::s2fdalt=0;
int IpsDisplay::prefalt=0;
int IpsDisplay::chargealt=-1;
int IpsDisplay::btqueue=-1;
int IpsDisplay::tempalt = -2000;
int IpsDisplay::mcalt = -100;
bool IpsDisplay::s2fmodealt = false;
int IpsDisplay::s2fclipalt = 0;
int IpsDisplay::iasalt = -1;
int IpsDisplay::yposalt = 0;
int IpsDisplay::tyalt = 0;
int IpsDisplay::pyalt = 0;
int IpsDisplay::wkalt = -3;
int IpsDisplay::wkspeeds[6];
ucg_color_t IpsDisplay::wkcolor;
char IpsDisplay::wkss[6];
int IpsDisplay::wkposalt;
int IpsDisplay::wkialt;

float IpsDisplay::_range_clip = 0;
int   IpsDisplay::_divisons = 5;
float IpsDisplay::_range = 5;
int IpsDisplay::average_climb = -100;
bool IpsDisplay::wkbox = false;
int  IpsDisplay::pref_qnh = 0;


IpsDisplay::IpsDisplay( Ucglib_ILI9341_18x240x320_HWSPI *aucg ) {
    ucg = aucg;
	_dtype = ILI9341;
	_divisons = 5;
	_range_clip = 0;
	_range = 5;
	tick = 0;
	_dc = GPIO_NUM_MAX;
	_reset = GPIO_NUM_MAX;
	_cs = GPIO_NUM_MAX;
}

IpsDisplay::~IpsDisplay() {
}


void IpsDisplay::drawArrowBox( int x, int y, bool arightside ){
	int fh = _getFontAscent();
	int fl = _getStrWidth("123");
	if( arightside )
		_drawTriangle( x+fl+4,y-(fh/2)-3,x+fl+4,y+(fh/2)+3,x+fl+4+fh/2,y );
	else
		_drawTriangle( x,y-(fh/2)-3,   x,y+(fh/2)+3,   x-fh/2,y );
}

void IpsDisplay::drawLegend( bool onlyLines ) {
	int hc=0;
	if( onlyLines == false ){
		_setFont(ucg_font_9x15B_mf);
		hc = _getFontAscent()/2;
	}
	_setColor(COLOR_WHITE);
	for( int i=_divisons; i >=-_divisons; i-- )
	{
		float legend = ((float)i*_range)/_divisons;  // only print the integers
		int y = (int)(dmid - int(legend*_pixpmd));
		if( onlyLines == false ){
			if( abs( legend  - int( legend )) < 0.1 ) {
				_setPrintPos(0, y+hc  );
				_printf("%+d",(int)legend );
			}
		}
		_drawHLine( DISPLAY_LEFT, y , 4 );
	}
}

// draw all that does not need refresh when values change


void IpsDisplay::writeText( int line, String text ){
	_setFont(ucg_font_ncenR14_hr);
	_setPrintPos( 1, 26*line );
	_setColor(COLOR_WHITE);
	_printf("%s",text.c_str());
}


void IpsDisplay::clear(){
	_setColor( COLOR_BLACK );
	_drawBox( 0,0,240,320 );
}

void IpsDisplay::bootDisplay() {
	ESP_LOGI(FNAME,"IpsDisplay::bootDisplay()");
	setup();
	if( display_type.get() == ST7789_2INCH_12P )
		_setRedBlueTwist( true );
	if( display_type.get() == ILI9341_TFT_18P )
		_invertDisplay( true );
	clear();
	if( display_orientation.get() == 1 )
		_setRotate180();

	_setColor(1, COLOR_BLACK );
	_setColor(0, COLOR_WHITE );
	_setFont(ucg_font_fub11_tr);
}


void IpsDisplay::initDisplay() {
	ESP_LOGI(FNAME,"IpsDisplay::initDisplay()");
	bootDisplay();
	_setPrintPos(0,YVAR-VARFONTH);
	_setColor(0, COLOR_HEADER );
	if( UNITVAR == 0 ) // m/s
		_print("   m/s ");
	if( UNITVAR == 1 ) // ft/min
		_print("100 ft/m");
	if( UNITVAR == 2 ) // knots
		_print("  Knots");
	_setPrintPos(FIELD_START,YVAR-VARFONTH);    // 65 -52 = 13

	_print("Average Vario");
	_setColor(0, COLOR_WHITE );

	// print TE scale
	drawLegend();

	_drawVLine( DISPLAY_LEFT+5,      VARBARGAP , DISPLAY_H-(VARBARGAP*2) );
	_drawHLine( DISPLAY_LEFT+5, VARBARGAP , bw+1 );
	_drawVLine( DISPLAY_LEFT+5+bw+1, VARBARGAP, DISPLAY_H-(VARBARGAP*2) );
	_drawHLine( DISPLAY_LEFT+5, DISPLAY_H-(VARBARGAP), bw+1 );

	// Sollfahrt Text
	_setFont(ucg_font_fub11_tr);
	fh = _getFontAscent();
	_setPrintPos(FIELD_START+6,YS2F-(2*fh)-8);
	_setColor(0, COLOR_HEADER );
	String iasu;
	if( UNITAS == 0 ) // km/h
 		iasu = "km/h";
 	if( UNITAS == 1 ) // mph
 		iasu = "mph";
 	if( UNITAS == 2 ) // knots
 		iasu = "kt";

 	if( airspeed_mode.get() == MODE_IAS )
 		_printf("IAS %s", iasu.c_str());
 	else if( airspeed_mode.get() == MODE_TAS )
 		_printf("TAS %s", iasu.c_str());

	_setPrintPos(ASVALX,YS2F-(2*fh)-8);
	_print(" S2F");

	_setColor(0, COLOR_WHITE );
	// AS Box
	int fl = 45; // _getStrWidth("200-");

	ASLEN = fl;
	S2FST = ASLEN+16;

    // S2F Zero
	// _drawTriangle( FIELD_START, dmid+5, FIELD_START, dmid-5, FIELD_START+5, dmid);
	_drawTriangle( FIELD_START+ASLEN-1, dmid, FIELD_START+ASLEN+5, dmid-6, FIELD_START+ASLEN+5, dmid+6);



	// Thermometer
	_drawDisc( FIELD_START+10, DISPLAY_H-4,  4, UCG_DRAW_ALL ); // white disk
	_setColor(COLOR_RED);
	_drawDisc( FIELD_START+10, DISPLAY_H-4,  2, UCG_DRAW_ALL );  // red disk
	_setColor(COLOR_WHITE);
	_drawVLine( FIELD_START-1+10, DISPLAY_H-20, 14 );
	_setColor(COLOR_RED);
	_drawVLine( FIELD_START+10,  DISPLAY_H-20, 14 );  // red color
	_setColor(COLOR_WHITE);
	_drawPixel( FIELD_START+10,  DISPLAY_H-21 );  // upper point
	_drawVLine( FIELD_START+1+10, DISPLAY_H-20, 14 );
	redrawValues();
}

void IpsDisplay::begin() {
	ESP_LOGI(FNAME,"IpsDisplay::begin");
	_begin(UCG_FONT_MODE_SOLID);
	setup();
}

void IpsDisplay::setup()
{
	ESP_LOGI(FNAME,"IpsDisplay::setup");
	_range = range.get();

	if( (int)_range <= 5 )
		_divisons = (int)_range*2;
	else if( (int)_range == 6 )
		_divisons = 6;
	else if( (int)_range == 7 )
		_divisons = 7;
	else if( (int)_range == 8 )
		_divisons = 4;
	else if( (int)_range == 9 )
		_divisons = 3;
	else if( (int)_range == 10 )
		_divisons = 5;
	else if( (int)_range == 12 )
			_divisons = 6;
	else if( (int)_range == 13 )
			_divisons = 5;
	else if( (int)_range == 14 )
			_divisons = 7;
	else if( (int)_range == 15 )
				_divisons = 3;
	else
		_divisons = 5;

	_pixpmd = (int)((  (DISPLAY_H-(2*VARBARGAP) )/2) /_range);
	ESP_LOGI(FNAME,"Pixel per m/s %d", _pixpmd );
	_range_clip = _range;
}

void IpsDisplay::drawGaugeTriangle( int y, int r, int g, int b, bool s2f ) {
	_setColor( r,g,b );
	if( s2f )
		_drawTriangle( DISPLAY_LEFT+4+bw+3+TRISIZE,  dmid+y,
						   DISPLAY_LEFT+4+bw+3, dmid+y+TRISIZE,
						   DISPLAY_LEFT+4+bw+3, dmid+y-TRISIZE );
	else
		_drawTriangle( DISPLAY_LEFT+4+bw+3,         dmid-y,
						   DISPLAY_LEFT+4+bw+3+TRISIZE, dmid-y+TRISIZE,
						   DISPLAY_LEFT+4+bw+3+TRISIZE, dmid-y-TRISIZE );
}


void IpsDisplay::drawAvgSymbol( int y, int r, int g, int b ) {
	int size = 6;
	_setColor( r,g,b );
	_drawTetragon( DISPLAY_LEFT+size-1, dmid-y,
					   DISPLAY_LEFT,        dmid-y+size,
					   DISPLAY_LEFT-size,   dmid-y,
					   DISPLAY_LEFT,        dmid-y-size
					 );
}



void IpsDisplay::redrawValues()
{
	ESP_LOGI(FNAME,"IpsDisplay::redrawValues()");
	chargealt = 101;
	tempalt = -2000;
	s2falt = -1;
	s2fdalt = -1;
	btqueue = -1;
	_te=-200;
	mcalt = -100;
	iasalt = -1;
	_ate = -200;
	prefalt = -1;
	pref_qnh = -1;
	tyalt = 0;
	for( int l=TEMIN-1; l<=TEMAX; l++){
		colors[l].color[0] = 0;
		colors[l].color[1] = 0;
		colors[l].color[2] = 0;
		colorsalt[l].color[0] = 0;
		colorsalt[l].color[1] = 0;
		colorsalt[l].color[2] = 0;
	}
	average_climb = -1000;
	wkalt = -3;
	wkspeeds[0] = 220;
    wkspeeds[1] = flap_minus_2.get();
    wkspeeds[2] = flap_minus_1.get();
    wkspeeds[3] = flap_0.get();
    wkspeeds[4] = flap_plus_1.get();
    wkspeeds[5] = 60;
    wkbox = false;
    wkposalt = -100;
    wkialt = -3;
    tyalt = -1000;
}

void IpsDisplay::drawTeBuf(){
	for( int l=TEMIN+1; l<TEMAX; l++){
		if( colorsalt[l].color[0] != colors[l].color[0]  || colorsalt[l].color[1] != colors[l].color[1] || colorsalt[l].color[2] != colors[l].color[2])
		{
			_setColor( colors[l].color[0], colors[l].color[1], colors[l].color[2] );
			_drawHLine( DISPLAY_LEFT+6, l, bw );
			colorsalt[l] = colors[l];
		}
	}
}

void IpsDisplay::setTeBuf( int y1, int h, int r, int g, int b ){
	// if( h == 0 )
	// 	return;
		// clip values for max TE bar
	y1 = dmid+(dmid-y1);
	if( y1-h >= TEMAX )
		h = -(TEMAX-y1);
	if( y1-h < TEMIN )
        h = TEMIN+y1;

	if( h>=0 ) {
		while( h >=0  ) {
			if( y1-h < TEMAX+1+TEGAP )
			{
				colors[y1-h].color[0] = r;
				colors[y1-h].color[1] = g;
				colors[y1-h].color[2] = b;
			}
			h--;
		}
	}
	else {
		while( h < 0  ) {
			if( y1-h < TEMAX+1+TEGAP )
			{
				colors[y1-h].color[0] = r;
				colors[y1-h].color[1] = g;
				colors[y1-h].color[2] = b;
			}
			h++;
		}
	}
}

float wkRelPos( float wks, float minv, float maxv ){
	// ESP_LOGI(FNAME,"wks:%f min:%f max:%f", wks, minv, maxv );
	if( wks <= maxv && wks >= minv )
		return ((wks-minv)/(maxv-minv));
	else if( wks > maxv )
		return 1;
	else if( wks < minv )
		return 0.5;
	return 0.5;
}

int IpsDisplay::getWk( int wks )
{
	for( int wk=-2; wk<=2; wk++ ){
		if( wks <= wkspeeds[wk+2] && wks >=  wkspeeds[wk+3] )
			return wk;
	}
	if( wks < wkspeeds[5] )
		return 1;
	else if( wks > wkspeeds[0] )
		return -2;
	else
		return 1;
}

void IpsDisplay::drawWkBar( int ypos, float wkf ){
	_setFont(ucg_font_profont22_mr );
	int lfh = _getFontAscent()+4;
	int lfw = _getStrWidth( "+2" );
	int top = ypos-lfh/2;
	if( !wkbox ) {
		_drawFrame(DISPLAY_W-lfw-5, top-3, lfw+4, 2*lfh);
		int tri = ypos+lfh/2-3;
		_drawTriangle( DISPLAY_W-lfw-10, tri-5,  DISPLAY_W-lfw-10,tri+5, DISPLAY_W-lfw-5, tri );
		wkbox = true;
	}
	_setClipRange( DISPLAY_W-lfw-2, top-2, lfw, 2*lfh-2 );
	for( int wk=int(wkf-1); wk<=int(wkf+1) && wk<=2; wk++ ){
		if(wk<-2)
			continue;
		if( wk == 0 )
			sprintf( wkss,"% d", wk);
		else
			sprintf( wkss,"%+d", wk);
		int y=top+(lfh+4)*(5-(wk+2))+(int)((wkf-2)*(lfh+4));
		_setPrintPos(DISPLAY_W-lfw-2, y );
		if( wk == 0 )
			_setColor(COLOR_WHITE);
		else
			_setColor(COLOR_WHITE);
		_printf(wkss);
		if( wk != -2 ) {
			_setColor(COLOR_WHITE);
			_drawHLine(DISPLAY_W-lfw-5, y+3, lfw+4 );
		}
	}
	_undoClipRange();
}

#define DISCRAD 3
#define BOXLEN  12
#define FLAPLEN 14
#define WKSYMST DISPLAY_W-28

void IpsDisplay::drawWkSymbol( int ypos, int wk, int wkalt ){
	_setColor( COLOR_WHITE );
	_drawDisc( WKSYMST, ypos, DISCRAD, UCG_DRAW_ALL );
	_drawBox( WKSYMST, ypos-DISCRAD, BOXLEN, DISCRAD*2+1  );
	_setColor( COLOR_BLACK );
	_drawTriangle( WKSYMST+DISCRAD+BOXLEN-2, ypos-DISCRAD,
					   WKSYMST+DISCRAD+BOXLEN-2, ypos+DISCRAD+1,
					   WKSYMST+DISCRAD+BOXLEN-2+FLAPLEN, ypos+wkalt*4 );
	_setColor( COLOR_RED );
	_drawTriangle( WKSYMST+DISCRAD+BOXLEN-2, ypos-DISCRAD,
					   WKSYMST+DISCRAD+BOXLEN-2, ypos+DISCRAD+1,
					   WKSYMST+DISCRAD+BOXLEN-2+FLAPLEN, ypos+wk*4 );
}


void IpsDisplay::drawDisplay( int ias, float te, float ate, float polar_sink, float altitude,
		                      float temp, float volt, float s2fd, float s2f, float acl, bool s2fmode, bool standard_alt ){
	if( _menu )
			return;
	// ESP_LOGI(FNAME,"IpsDisplay::drawDisplay  TE=%0.1f", te);
	xSemaphoreTake(spiMutex,portMAX_DELAY );
	tick++;

	// S2F given im km/h: Unit adaption for mph and knots
 	if( UNITVAR == 1 ){  // mph
 		s2f = s2f*0.621371;
 		s2fd = s2fd*0.621371;
 	}
 	if( UNITVAR == 2 ){ // knots
 		s2f = s2f*0.539957;
 		s2fd = s2fd*0.539957;
 	}


	// WK-Indicator
	if( flap_enable.get() && ias != iasalt )
	{
		float wkspeed = ias * sqrt( 100.0/( ballast.get() +100.0) );
		int wki = getWk( wkspeed );
	    float wkpos=wkRelPos( wkspeed, wkspeeds[wki+3], wkspeeds[wki+2] );
	    int wk = (int)((wki - wkpos + 0.5)*10);
	    if( wkposalt != wk ) {
	    	// ESP_LOGI(FNAME,"ias:%d wksp:%f wki:%d wk:%d wkpos%f", ias, wkspeed, wki, wk, wkpos );
	    	_setColor(  COLOR_WHITE  );
	    	drawWkBar( YS2F-fh, (float)(wk)/10 );
	    	wkposalt = wk;
	    }
	    if( wki != wkialt ) {
	    	drawWkSymbol( YS2F-fh-25, wki, wkialt );
	    	wkialt=wki;
	    }
	}

	_setFont(ucg_font_fub35_hn);  // 52 height
	_setColor(  COLOR_WHITE  );

    // Average Vario
	if( _ate != (int)(ate*10) ) {
		if( ate < 0 ) {
			// erase V line from +
			_setColor( COLOR_BLACK );
			_drawVLine( FIELD_START+PMLEN/2-1, YVARMID-PMLEN/2, PMLEN );
			_drawVLine( FIELD_START+PMLEN/2, YVARMID-PMLEN/2, PMLEN );
			_drawVLine( FIELD_START+PMLEN/2+1, YVARMID-PMLEN/2, PMLEN );
			// draw just minus
			_setColor(  COLOR_WHITE  );
			_drawHLine( FIELD_START, YVARMID-1, PMLEN );
			_drawHLine( FIELD_START, YVARMID, PMLEN );
			_drawHLine( FIELD_START, YVARMID+1, PMLEN );
		}
		else {
			// draw just plus
			_drawHLine( FIELD_START, YVARMID-1, PMLEN );
			_drawHLine( FIELD_START, YVARMID, PMLEN );
			_drawHLine( FIELD_START, YVARMID+1, PMLEN );
			_drawVLine( FIELD_START+PMLEN/2-1, YVARMID-PMLEN/2, PMLEN );
			_drawVLine( FIELD_START+PMLEN/2, YVARMID-PMLEN/2, PMLEN );
			_drawVLine( FIELD_START+PMLEN/2+1, YVARMID-PMLEN/2, PMLEN );
		}
		_setPrintPos(FIELD_START+SIGNLEN,YVAR);
		float tep=ate;
		if( tep < 0 )
			tep=-ate;

		if     ( UNITVAR == 0 )
			_printf("%0.1f  ", tep);
		else if(  UNITVAR == 1 ){
			int fpm = (int((tep*196.85)+0.5)/10)*10;
			if( abs(fpm) > 999 ) {
				_setPrintPos(FIELD_START+SIGNLEN,YVAR-8);
				_setFont(ucg_font_fub25_hr);
			}
			_printf("%d   ", fpm );  // ft/min
		}
		else if(  UNITVAR == 2 )
			_printf("%0.1f  ", tep*1.94384 );         // knots

		String units;
		_setFont(ucg_font_fub11_hr);
		if     ( UNITVAR == 0 )
			units = "m/s";
		else if(  UNITVAR == 1 )
			units="ft/m";
		else if(  UNITVAR == 2 )
			units="kt ";
		int mslen = _getStrWidth(units.c_str());
		_setPrintPos(DISPLAY_W-mslen,YVAR-12);
		_print(units.c_str());
		_ate = (int)(ate)*10;
	}

	// Altitude Header
	int qnh = (int)(QNH.get() +0.5 );
	if( standard_alt )
		qnh = 1013;

	if( qnh != pref_qnh ){
		_setFont(ucg_font_fub11_tr);
		_setPrintPos(FIELD_START,YALT-S2FFONTH);
		if( standard_alt ) {
			_setColor(0, COLOR_BLACK );
			_printf("Altitude QNH %d ", pref_qnh );
			_setPrintPos(FIELD_START,YALT-S2FFONTH);
			_setColor(0, COLOR_HEADER );
			_printf("Altitude QNE %d ", qnh );
		}
		else {
			_setColor(0, COLOR_BLACK );
			_printf("Altitude QNE %d ", pref_qnh );
			_setPrintPos(FIELD_START,YALT-S2FFONTH);
			_setColor(0, COLOR_HEADER );
			_printf("Altitude QNH %d ", qnh );
		}
		pref_qnh = qnh;
	}
	// Altitude
	int alt = (int)(altitude+0.5);
	if( alt != prefalt ) {
		_setColor(  COLOR_WHITE  );
		_setPrintPos(FIELD_START,YALT);
		_setFont(ucg_font_fub25_hr);
		if( UNITALT == 0 ) { //m
			_printf("  %-4d m ", alt  );
		}
		if( UNITALT == 1 ){ //feet
			_printf("  %-4d ft ", int((altitude*3.28084) + 0.5)  );
		}
		if( UNITALT == 2 ){ //FL
			_printf("FL %-4d  ", int((altitude*0.0328084) + 0.5)  );
		}
		prefalt = alt;
	}
	// MC Value
	int aMC = MC.get() * 10;
	if( aMC != mcalt ) {
			_setFont(ucg_font_fub11_hr);
			_setPrintPos(5,DISPLAY_H-8);
			_setColor(COLOR_HEADER);
			_printf("MC:");
			_setPrintPos(5+_getStrWidth("MC:"),DISPLAY_H-4);
			_setColor(COLOR_WHITE);
			_setFont(ucg_font_fur14_hf);
			_printf("%1.1f", MC.get() );
			mcalt=aMC;
		}

    // Temperature Value
	if( (int)(temp*10) != tempalt ) {
		_setFont(ucg_font_fur14_hf);
		_setPrintPos(FIELD_START+20,DISPLAY_H);
		if( temp != DEVICE_DISCONNECTED_C )
			_printf("%-2.1f\xb0""  ", temp );
		else
			_printf(" ---   ");
		tempalt=(int)(temp*10);
	}
	// Battery Symbol
#define BATX (DISPLAY_W-15)
#define BATY (DISPLAY_H-12)
	int chargev = (int)( volt *10 );
	if ( chargealt != chargev ) {
		charge = (int)(( volt -  bat_low_volt.get() )*100)/( bat_full_volt.get() - bat_low_volt.get() );
		if(charge < 0)
			charge = 0;
		if( charge > 100 )
			charge = 100;
		if( (tick%100) == 0 )  // check setup changes all 10 sec
		{
			yellow =  (int)(( bat_yellow_volt.get() - bat_low_volt.get() )*100)/( bat_full_volt.get() - bat_low_volt.get() );
			red = (int)(( bat_red_volt.get() - bat_low_volt.get() )*100)/( bat_full_volt.get() - bat_low_volt.get() );
		}
		_drawBox( BATX-40,BATY-2, 36, 12  );  // Bat body square
		_drawBox( BATX-4, BATY+1, 3, 6  );      // Bat pluspole pimple
		if ( charge > yellow )  // >25% grün
			_setColor( COLOR_GREEN ); // green
		else if ( charge < yellow && charge > red )
			_setColor( COLOR_YELLOW ); //  yellow
		else if ( charge < red )
			_setColor( COLOR_RED ); // red
		int chgpos=(charge*32)/100;
		if(chgpos <= 4)
			chgpos = 4;
		_drawBox( BATX-40+2,BATY, chgpos, 8  );  // Bat charge state
		_setColor( DARK_GREY );
		_drawBox( BATX-40+2+chgpos,BATY, 32-chgpos, 8 );  // Empty bat bar
		_setColor( COLOR_WHITE );
		_setFont(ucg_font_fub11_hr);
		_setPrintPos(BATX-40,BATY-8);
		if( battery_display.get() == 0 )
			_printf("%3d%%  ", charge);
		else {
			_setPrintPos(BATX-50,BATY-8);
			_printf("%2.1f V", volt);
		}
		chargealt = chargev;
	}
	if( charge < red ) {  // blank battery for blinking
		if( (tick%10) == 0 ) {
			_setColor( COLOR_BLACK );
			_drawBox( BATX-40,BATY-2, 40, 12  );
		}
		if( ((tick+5)%10) == 0 )  // trigger redraw
		{
			chargealt++;
		}
	}

	// Bluetooth Symbol
	int btq=BTSender::queueFull();
	if( btq != btqueue )
	{
		if( blue_enable.get() ) {
			ucg_int_t btx=DISPLAY_W-22;
			ucg_int_t bty=BTH/2;
			if( btq )
				_setColor( COLOR_MGREY );
			else
				_setColor( COLOR_BLUE );  // blue

			_drawRBox( btx-BTW/2, bty-BTH/2, BTW, BTH, BTW/2-1);

			// inner symbol
			_setColor( COLOR_WHITE );
			_drawTriangle( btx, bty, btx+BTSIZE, bty-BTSIZE, btx, bty-2*BTSIZE );
			_drawTriangle( btx, bty, btx+BTSIZE, bty+BTSIZE, btx, bty+2*BTSIZE );
			_drawLine( btx, bty, btx-BTSIZE, bty-BTSIZE );
			_drawLine( btx, bty, btx-BTSIZE, bty+BTSIZE );
		}
		btqueue = btq;
	}

 	int s2fclip = s2fd;
	if( s2fclip > MAXS2FTRI )
		s2fclip = MAXS2FTRI;
	if( s2fclip < -MAXS2FTRI )
		s2fclip = -MAXS2FTRI;

 	int ty = 0;
 	if( UNITVAR == 0 )  // m/s
 		ty = (int)(te*_pixpmd);         // 1 unit = 1 m/s
 	else if( UNITVAR == 1 )
 		ty = (int)(te*_pixpmd*1.9685);  // 1 unit = 100 ft/min
 	else if( UNITVAR == 2 )
 		ty = (int)(te*_pixpmd*1.94384); // 1 unit = 1 kt

 	int py = (int)(polar_sink*_pixpmd);
    // Gauge Triangle
	if( s2fmode !=  s2fmodealt ){
 		drawGaugeTriangle( tyalt, COLOR_BLACK );
 		drawGaugeTriangle( s2fclipalt, COLOR_BLACK, true );
 		drawGaugeTriangle( ty, COLOR_BLACK );
 		drawGaugeTriangle( s2fclip, COLOR_BLACK, true );
 		s2fmodealt = s2fmode;
 	}

	if ( average_climb !=  (int)(acl*10) ){
		drawAvgSymbol(  (average_climb*_pixpmd)/10, COLOR_BLACK );
		drawLegend( true );
		average_climb = (int)(acl*10);
		drawAvgSymbol(  (average_climb*_pixpmd)/10, COLOR_RED );
	}

 	// TE Stuff
 	if( ty != tyalt || py != pyalt )
 	{
 		// setTeBuf(  dmid, _range*_pixpmd+1, COLOR_BLACK );
 		// setTeBuf(  dmid, -(_range*_pixpmd+1), COLOR_BLACK );
 		setTeBuf(  dmid, MAXTEBAR, COLOR_BLACK );
 		setTeBuf(  dmid, -MAXTEBAR, COLOR_BLACK );

 		if( ps_display.get() )
 			setTeBuf(  dmid, py, COLOR_BLUE );
 		if( ty > 0 ){
 			setTeBuf(  dmid, ty, COLOR_GREEN );
 			if( ps_display.get() )
 				setTeBuf(  dmid, py, COLOR_GREEN );
 		}
 		else {
 			if( ps_display.get() ) {
				if( ty > py ){
					setTeBuf(  dmid, ty, COLOR_BLUE );
					setTeBuf(  dmid+ty, py-ty, COLOR_GREEN );
				}
				else
				{
					 setTeBuf(  dmid, py, COLOR_BLUE );
					 setTeBuf(  dmid+py, ty-py, COLOR_RED );
				}
 			}else
 				setTeBuf(  dmid, ty, COLOR_RED );
 		}
 		drawTeBuf();

		// Small triangle pointing to actual vario value
		if( !s2fmode ){
			// First blank the old one
			drawGaugeTriangle( tyalt, COLOR_BLACK );
			drawGaugeTriangle( ty, COLOR_WHITE );
		}
	    tyalt = ty;
	    pyalt = py;
	    // vTaskDelay(3);

	}


    // AS
 	if( iasalt != ias ) {
 		// draw new
 		int iasp=0;
 		if( UNITVAR == 0 ) // km/h
 			iasp = ias;
 		if( UNITVAR == 1 ) // mph
 			iasp = ias*0.621371;
 		if( UNITVAR == 2 ) // knots
 			iasp = ias*0.539957;

 		_setColor(  COLOR_WHITE  );
 		// print speed values bar
 		_setFont(ucg_font_fub11_hn);
 		_drawFrame( FIELD_START, dmid-(MAXS2FTRI)-4, ASLEN+6, (MAXS2FTRI*2)+8 );
 		_setClipRange( FIELD_START, dmid-(MAXS2FTRI), ASLEN+6, (MAXS2FTRI*2) );
 		for( int speed = iasp-MAXS2FTRI-(fh); speed<iasp+MAXS2FTRI+(fh); speed++ )
 		{
			if( (speed%20) == 0 && (speed >= 0) ) {
				// blank old values
				_setColor( COLOR_BLACK );
				if( speed == 0 )
					_drawBox( FIELD_START+6,dmid+(speed-iasp)-(fh/2)-19, ASLEN-6, fh+25 );
				else
					_drawBox( FIELD_START+6,dmid+(speed-iasp)-(fh/2)-9, ASLEN-6, fh+15 );
				int col = abs(((speed-iasp)*2));
				_setColor(  col,col,col  );
				_setPrintPos(FIELD_START+8,dmid+(speed-iasp)+(fh/2));
				_printf("%3d ""-", speed);
			}
 		}
 		_undoClipRange();
 		// AS cleartext
 		_setFont(ucg_font_fub14_hn);
 		_setPrintPos(FIELD_START+8, YS2F-fh );
 		_setColor(  COLOR_WHITE  );
		_printf("%3d ", iasp);
		iasalt = ias;
 	}
 	// S2F command trend triangle
 	if( (int)s2fd != s2fdalt ) {

        // Arrow pointing there
		if( s2fmode ){
			// erase old
			drawGaugeTriangle( s2fclipalt, COLOR_BLACK, true );
			// Draw a new one at current position
			drawGaugeTriangle( s2fclip, COLOR_WHITE, true );
			_setColor(  COLOR_WHITE  );
		}
		// S2F value
		_setColor(  COLOR_WHITE  );
 		_setFont(ucg_font_fub14_hn);
		int fa=_getFontAscent();
		int fl=_getStrWidth("100");
		_setPrintPos(ASVALX, YS2F-fh);
		_printf("%3d  ", (int)(s2falt+0.5)  );
		// draw S2F Delta
		// erase old
		_setColor(  COLOR_BLACK  );
		char s[10];
		sprintf(s,"%+3d  ",(int)(s2fdalt+0.5));
		fl=_getStrWidth(s);
		_setPrintPos( FIELD_START+S2FST+(S2F_TRISIZE/2)-fl/2-5,yposalt );
		_printf(s);
		int ypos;
		if( s2fd < 0 )
			ypos = dmid+s2fclip-2;  // slower, up
		else
			ypos = dmid+s2fclip+2+fa;
        // new S2F Delta val
		if( abs(s2fd) > 10 ) {
			_setColor(  COLOR_WHITE  );
			sprintf(s," %+3d  ",(int)(s2fd+0.5));
			fl=_getStrWidth(s);
			_setPrintPos( FIELD_START+S2FST+(S2F_TRISIZE/2)-fl/2,ypos );
			_printf(s);
		}
		yposalt = ypos;
 		_setClipRange( FIELD_START+S2FST, dmid-MAXS2FTRI, S2F_TRISIZE, (MAXS2FTRI*2)+1 );
 		bool clear = false;
 		if( s2fd > 0 ) {
 			if ( (int)s2fd < s2fdalt || (int)s2fdalt < 0 )
 				clear = true;
 		}
 		else {
 			if ( (int)s2fd > s2fdalt || (int)s2fdalt > 0  )
 		 		clear = true;
 		}
 		// clear old triangle for S2F
 		if( clear ) {
			_setColor( COLOR_BLACK );
			_drawTriangle( FIELD_START+S2FST, dmid,
							   FIELD_START+S2FST+(S2F_TRISIZE/2), dmid+(int)s2fd,
							   FIELD_START+S2FST+(S2F_TRISIZE/2), dmid+(int)s2fdalt );
			_drawTriangle( FIELD_START+S2FST+S2F_TRISIZE, dmid,
							   FIELD_START+S2FST+(S2F_TRISIZE/2), dmid+(int)s2fd,
							   FIELD_START+S2FST+(S2F_TRISIZE/2), dmid+(int)s2fdalt );
 		}
 		// draw new S2F command triangle
		if( s2fd < 0 )
			_setColor( LIGHT_GREEN );
		else
			_setColor( COLOR_RED );
		_drawTriangle( FIELD_START+S2FST, dmid,
						   FIELD_START+S2FST+S2F_TRISIZE, dmid,
						   FIELD_START+S2FST+(S2F_TRISIZE/2), dmid+(int)s2fd );

 		_undoClipRange();
 		// green bar for optimum speed within tacho
 		_setClipRange( FIELD_START, dmid-(MAXS2FTRI), ASLEN+6, (MAXS2FTRI*2) );
 		_setColor( COLOR_BLACK );
 	 	_drawBox( FIELD_START+1,dmid+s2fdalt-16, 6, 32 );
 	 	_setColor( COLOR_GREEN );
 	 	_drawBox( FIELD_START+1,dmid+s2fd-15, 6, 30 );
 	 	_undoClipRange();
 		s2fdalt = (int)s2fd;
 		s2falt = (int)(s2f+0.5);
		s2fclipalt = s2fdalt;
		if( s2fclipalt > MAXS2FTRI )
				s2fclipalt = MAXS2FTRI;
			if( s2fclipalt < -MAXS2FTRI )
				s2fclipalt = -MAXS2FTRI;
 	}
 	_setColor(  COLOR_WHITE  );
 	_drawHLine( DISPLAY_LEFT+6, dmid, bw );
 	xSemaphoreGive(spiMutex);
}


