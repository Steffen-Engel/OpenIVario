/*
 * Polars.cpp
 *
 *  Created on: Mar 1, 2019
 *      Author: iltis
 *
 *      Class to deliver a library of default polars
 */

#include "Polars.h"
#include <logdef.h>
#include "SetupNG.h"

// Format per glider:  { Index, GliderType, Reference Wingload (kg/m2), speed1 (km/h), sink1 (m/s), speed2, sink2, speed3, sink3 , max ballast [liters or kg], wing area [m2] },

static const t_polar polars_default_arr[] = {
	{ 1000, "User Polar",34.40,80,-0.66,125,-0.97,175,-2.24,160,10.5},
	{ 1010, "Antares 20E", 42.86,100,-0.53,140,-0.76,200,-1.79,216,12.6},
	{ 1020, "Apis 2", 25.8,80,-0.60,100,-0.75,142,-1.50,0,12.4},
	{ 1030, "Arcus", 44.9,110,-0.64,140,-0.88,180,-1.47,185,15.59},
	{ 1050, "ASK 21",31.30,80,-0.76,125,-1.21,175,-2.75,0,17.95},
	{ 1060, "ASK 23",26.30,80,-0.69,125,-1.32,175,-2.93,0,12.9},
	{ 1070, "ASK 13",21.71,70,-0.80,100,-1.20,150,-2.80,0,17.5},
	{ 1080, "ASH 25e",43.00,80,-0.62,125,-0.72,175,-1.49,120,16.31},
	{ 1090, "ASH 31Mi/18",42.3,90,-0.45,150,-0.97,200,-2.02,120,11.83},
	{ 1100, "ASH 31Mi/21",38.8,90,-0.55,150,-0.99,200,-2.32,140,13.16},
	{ 1110, "ASW 15B",27.7,80,-0.63,120,-1.16,150,-2.00,90,11.0},
	{ 1120, "ASW 17",32.00,80,-0.55,125,-0.82,175,-1.78,100,14.84},
	{ 1130, "ASW 19",31.50,80,-0.63,125,-1.09,175,-2.44,100,10.5},
	{ 1140, "ASW 20",33.60,80,-0.62,125,-0.92,175,-1.83,120,10.5},
	{ 1150, "ASW 20A",35.90,80,-0.69,125,-1.05,175,-2.25,120,10.5},
	{ 1160, "ASW 20C",33.25,80,-0.61,125,-0.94,175,-1.88,120,10.5},
	{ 1170, "ASW 20L",32.30,80,-0.57,125,-0.93,175,-2.01,120,11.05},
	{ 1180, "ASW 22/24",32.80,80,-0.43,125,-0.85,175,-1.77,240,14.9},
	{ 1190, "ASW 22M",40.20,80,-0.51,125,-0.75,175,-1.60,185,16.31},
	{ 1200, "ASW 24",35.0,108.82,-0.728,142.25,-1.213,167.4,-1.766,155,10.0},
	{ 1210, "ASW 24WL",35.7,108.80,-0.635,156.40,-1.182,211,-2.540,155,10.0},
	{ 1220, "ASW 28",31.42,90,-0.57,130,-1.05,170,-2.15,190,10.5},
	{ 1230, "B4 P-11",24.9,80,-0.65,110,-1.66,150,-2.38,0,14.05},
	{ 1240, "Blanik L13",25.8,80,-0.70,150,-2.45,200,-5.0,0,19.15},
	{ 1250, "Calif A21S",39.20,80,-0.69,125,-0.93,175,-1.82,0,23.5},
	{ 1260, "Cirrus Std",30.60,80,-0.64,125,-1.18,175,-2.59,60,10.04},
	{ 1270, "Cirrus 17.7m",28.50,75,-0.5,120,-1.0,160,-1.83,98,12.6},
	{ 1280, "DG 100",30.00,80,-0.62,125,-1.16,175,-2.68,100,11.0},
	{ 1290, "DG 100G",30.48,80,-0.64,125,-1.13,175,-2.50,100,11.0},
	{ 1300, "DG 200",34.00,80,-0.65,125,-0.97,175,-2.05,120,10.0},
	{ 1310, "DG 300",33.20,80,-0.60,125,-1.02,175,-2.48,130,10.28},
	{ 1320, "DG 400/17",38.40,80,-0.55,125,-1.01,175,-2.12,90,10.0},
	{ 1330, "DG 500M/22",40.0,80,-0.51,142,-1.0,180,-1.75,100,18.29},
	{ 1340, "DG 600/15",33.3,85,-0.57,130,-1.0,180,-1.86,187,10.95},
	{ 1350, "DG 600/18",32.0,80,-0.50,136,-1.0,180,-1.80,187,11.81},
	{ 1360, "DG 800A/15",43.82,133.90,-0.879,178.87,-1.528,223.59,-2.527,100,10.68},
	{ 1370, "DG 800S/15",43.82,133.90,-0.879,178.87,-1.528,223.59,-2.527,180,10.68},
	{ 1380, "DG 808B/18",39.96,106.00,-0.620,171.75,-1.466,214.83,-2.346,100,11.81},
	{ 1390, "DG 800S/18",39.96,106.00,-0.620,171.75,-1.466,214.83,-2.346,237,11.81},
	{ 1400, "DG 800C/18",39.96,106.00,-0.620,171.75,-1.466,214.83,-2.346,150,11.81},
	{ 1410, "DG 1000S/20",28.00,80,-0.51,125,-0.93,175,-2.80,160,17.53},
	{ 1420, "Diamant 18",28,72,-0.52,125,-0.95,160,-1.75,50,14.28},
	{ 1430, "Discus A",29.67,80,-0.56,125,-1.00,175,-2.55,200,10.58},
	{ 1440, "Discus 2B",51.7,100,-0.75,125,-0.80,175,-1.50,200,10.16},
	{ 1450, "DUO Discus",30.5,80,-0.52,130,-1.00,177,-2.50,198,16.4},
	{ 1460, "Elfe S4D",27.00,80,-0.69,125,-1.23,175,-2.76,0,11.8},
	{ 1470, "Genesis II",33.54,94.00,-0.608,141.05,-1.178,172.40,-1.960,0,11.5},
	{ 1480, "G-102 CS",30.00,80,-0.61,125,-1.07,175,-2.45,100,12.4},
	{ 1490, "G-102 CS Top",34.75,80,-0.70,125,-1.18,175,-2.55,0,12.4},
	{ 1500, "G-102 Club",29.00,80,-0.70,125,-1.18,175,-2.55,0,12.4},
	{ 1510, "G-102 3B",27.60,80,-0.68,125,-1.34,175,-2.84,0,12.4},
	{ 1520, "G-102 Twin",33.40,80,-0.78,125,-1.07,175,-2.17,0,17.8},
	{ 1530, "G-103 Twin",27.60,80,-0.75,120,-1.0,200,-3.4,0,17.52},
	{ 1530, "H-304/17",35.20,80,-0.53,125,-0.93,175,-1.93,115,10.68},
	{ 1540, "H-304",35.80,80,-0.64,125,-0.96,175,-2.04,115,9.88},
	{ 1550, "Hornet C",32.30,80,-0.63,125,-1.12,175,-2.55,100,9.8},
	{ 1560, "Hornet H-206",32.05,80,-0.64,125,-1.12,175,-2.49,100,9.8},
	{ 1570, "Jantar 2B",33.20,80,-0.51,125,-0.87,175,-2.02,167,14.3},
	{ 1580, "Jantar Std",33.50,80,-0.68,125,-1.07,175,-2.25,100,14.24},
	{ 1590, "Jantar Std 3",30.58,95,-0.66,180,-2.25,220,-3.85,150,10.66},
	{ 1600, "Jantar SZD 41A",30.90,80,-0.62,125,-1.10,175,-2.41,0,10.66},
	{ 1610, "Janus",31.10,80,-0.69,125,-1.04,175,-2.08,240,16.6},
	{ 1620, "Janus CM",39.0,90,-0.64,120,-0.78,180,-1.80,0,17.3},
	{ 1630, "Jeans Astir ",29.68,80,-0.73,125,-1.27,175,-2.78,0,12.4},
	{ 1640, "JS1 18m",36.0,110,-0.55,160,-1.20,200,-2.2,190,11.19},
	{ 1650, "JS1 21m",36.0,110,-0.5,160,-1.18,200,-2.2,214,12.25},
	{ 1660, "Ka 6b R/S ",22.20,80,-0.76,125,-1.75,175,-4.30,0,12.4},
	{ 1670, "Ka 6 CR",   24.19,80,-0.70,125,-1.68,150,-2.70,0,12.4},
	{ 1680, "Ka 7",26.60,80,-0.94,125,-1.80,175,-4.55,0,17.4},
	{ 1690, "Ka 8",21.02,60,-0.65,120,-2.10,150,-3.80,0,14.15},
	{ 1700, "Kestrel 17",31.70,80,-0.59,125,-0.98,175,-2.01,50,11.58},
	{ 1710, "Kestrel 19",33.40,80,-0.57,125,-0.93,175,-2.03,100,12.87},
	{ 1720, "Kestrel 22",35.80,80,-0.53,125,-0.81,175,-1.74,100,16.26},
	{ 1730, "Standard Libelle",35.7,80,-0.68,120,-1.1,175,-2.6,50,9.8},
	{ 1740, "Club Libelle 205",33.7,80,-0.66,124,-1.2,176,-2.77,0,9.8},
	{ 1750, "LAK-12 (20.4m)",29.4,75,-0.48,120,-0.8,180,-1.92,190,14.63},
	{ 1760, "LAK-17 (15m)",31.45,100,-0.60, 120,-0.72,150,-1.09,215,9.06 },
	{ 1780, "LAK-17a (15m)",31.45,95,-0.574,148,-1.310,200,-2.885,180, 9.06 },
	{ 1790, "LAK-17 (18m)",30.10, 100,-0.56, 120,-0.74,150,-1.16,205,9.8 },
	{ 1800, "LAK-17a (18m)",36.73,85,-0.50,140,-1.0,180,-1.82,180,9.80 },
	{ 1810, "Lambada (15m)",34.88,80,-1.0,140,-2.4,200,-4.8,0,12.9},
	{ 1820, "LS 3a",32.60,80,-0.63,125,-0.96,175,-2.9,150,10.5},
	{ 1830, "LS 1 f",35.50,80,-0.64,125,-1.07,175,-2.34,0,9.74},
	{ 1840, "LS 3 15m",35.50,80,-0.63,125,-0.92,175,-1.87,150,10.5},
	{ 1850, "LS 3 17m",33.00,80,-0.56,125,-0.94,175,-2.44,0,11.22},
	{ 1860, "LS 4",34.40,80,-0.66,125,-0.97,175,-2.24,160,10.5},
	{ 1870, "LS 6a",33.3,100,-0.67,155,-1.48,212,-3.00,140,10.5},
	{ 1880, "LS 6 18w",39.0,100,-0.68,150,-0.91,200,-2.25,140,11.42},
	{ 1890, "LS 7 WL",35.97,103.77,-0.73,155.65,-1.47,180.00,-2.66,150,9.73},
	{ 1900, "LS 8",34.28,100,-0.67,155,-1.45,185,-2.5,175,10.5},
	{ 1910, "LS 9",45.0,100,-0.69,150,-0.86,200,-1.75,140,11.42},
	{ 1920, "L-Spatz 55",22.22,80,-0.8,100,-1.25,125,-2.3,0,11.7},
	{ 1930, "Marianne",37.82,80,-0.5,140,-1.5,200,-3.6,0,17.185},
	{ 1940, "MG-23",25.3,78,-0.7,110,-1.4,140,-2.15,0,14.21},
	{ 1950, "Mini Nimbus",34.20,80,-0.63,125,-0.96,175,-2.13,125,9.86},
	{ 1960, "MiniLak FES",40.8,90,-0.69,120,-0.87,180,-2.06,80,8.41},
	{ 1970, "Mistral C",31.36,80,-0.67,125,-1.15,175,-2.64,0,10.9},
	{ 1980, "Mosquito",36.20,80,-0.63,125,-0.92,175,-1.99,115,9.86},
	{ 1990, "Mosquito B",34.00,80,-0.62,125,-0.93,175,-2.04,115,9.86},
	{ 2000, "Nimbus 2",30.00,80,-0.50,125,-0.88,175,-2.00,160,14.4},
	{ 2010, "Nimbus 3",29.50,80,-0.46,125,-0.88,175,-1.85,338,16.28},
	{ 2020, "Nimbus 3DM",48.17,95,-0.5,130,-0.70,187,-1.60,168,17.02},
	{ 2030, "Nimbus 3/24.5",28.90,80,-0.39,125,-0.86,175,-1.87,338,16.7},
	{ 2040, "Nimbus 4/26.5",33.42,85.1,-0.41,127.98,-0.75,162.74,-1.4,300,17.86},
	{ 2050, "Phoebus B",24.6,80,-0.6,125,-1.1,175,-2.4,80,13.16},
	{ 2060, "Piccolo B",26.89,68,-0.9,78,-1.00,111,-2.0,0,10.6},
	{ 2070, "Pegase 101A",32.80,80,-0.63,125,-1.08,175,-2.54,100,12.0},
	{ 2080, "Pik 20B",35.40,102.5,-0.69,157.76,-1.59,216.91,-3.6,140,10.0},
	{ 2090, "Pik 20E",43.7,109.61,-0.83,166.68,-2,241.15,-4.7,120,10.0},
	{ 2100, "Pik 20D",32.50,80,-0.66,125,-1.06,175,-2.20,140,10.0},
	{ 2110, "Salto 13,6m",31.60,80,-0.72,125,-1.32,175,-2.96,0,8.58},
	{ 2120, "Salto 15m",31.20,80,-0.62,125,-1.24,175,-2.75,0,9.14},
	{ 2130, "Salto 15,5m",34.00,80,-0.62,125,-1.21,175,-2.73,0,9.14},
	{ 2140, "SB-10",36.70,80,-0.48,125,-0.82,175,-1.74,100,22.95},
	{ 2150, "SB-11",36.90,80,-0.68,125,-0.91,175,-1.84,105,10.56},
	{ 2160, "SB-12",33.20,80,-0.60,125,-0.99,175,-2.36,150,10.02},
	{ 2170, "SF 25B", 30.9,75,-1.13,111,-2.1,135,-3.1,0,17.5},
	{ 2180, "SF 27",26.00,74,-0.64,105,-1.00,175,-2.50,0,12.0},
	{ 2190, "SF 27M",30.00,80,-0.72,115,-1.00,175,-2.20,0,12.0},
	{ 2200, "SF 34",36.10,80,-0.90,125,-1.19,175,-2.54,0,14.8},
	{ 2210, "SFS 31",35.8,85,-0.85,127,-1.5,150,-2.15,0,12.00},
	{ 2220, "Silent Club",25.2,65,-0.62,115,-1.05,175,-3.2,0,10.3},
	{ 2225, "Stratos II",17.17,50,-0.95,65,-1.13,100,-2.5,50,16.3},
	{ 2230, "Sunrise II",26.0,65,-1.2,100,-1.8,130,-3.0,0,15.4},
	{ 2240, "SZD51-1 Jun.",30.38,80,-0.65,125,-1.20,175,-2.80,0,12.51},
	{ 2250, "SZD55",52.08,85,-0.9,125,-0.86,175,-1.50,195,9.6},
	{ 2260, "Taurus", 37.37,94,-0.7,108,-0.75,180,-2.2, 0, 12.62 },
	{ 2270, "Ventus A",35.00,80,-0.60,125,-0.87,175,-1.85,168,9.51},
	{ 2280, "Ventus A-B/16.6",32.50,80,-0.53,125,-0.93,175,-1.97,168,9.96},
	{ 2290, "Ventus B",35.00,80,-0.58,125,-0.88,175,-1.93,168,9.51},
	{ 2300, "Ventus C",33.00,80,-0.52,120,-0.75,180,-2.00,168,10.15},
	{ 2310, "VSO 10",28.90,83,-0.65,120,-1.05,160,-2.28,0,12.00}
};


const t_polar Polars::getPolar( int x ) {
	return polars_default_arr[x];
}

int Polars::numPolars() {
	return(  sizeof( polars_default_arr ) / sizeof(t_polar) );
}

Polars::Polars(){ }


Polars::~Polars() {
	// TODO Auto-generated destructor stub
}

void Polars::begin(){
	ESP_LOGI( FNAME,"Polars::begin() configured glider type:%d, index:%d", glider_type.get(), glider_type_index.get() );
	if( glider_type_index.get() == 0 ){
		ESP_LOGI( FNAME,"Need first to initialize unique glider type index: %d", polars_default_arr[ glider_type.get() ].index );
		glider_type_index.set(  polars_default_arr[ glider_type.get() ].index );
	}
    int unique_glider_index = glider_type_index.get();
	if( polars_default_arr[ glider_type.get() ].index != unique_glider_index ){
		ESP_LOGI( FNAME,"Unique index missmatch, migrate index after polar DB change");
		for( int p=0; p<numPolars(); p++ ){
			if( polars_default_arr[p].index == unique_glider_index ){
				ESP_LOGI( FNAME,"Found Glider index %d at new position %d (old:%d)", unique_glider_index, p, glider_type.get() );
				if( glider_type.get() != p )
					glider_type.set( p );
				break;
			}
		}
	}
}


