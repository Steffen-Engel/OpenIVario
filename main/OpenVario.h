/*
 * OpenVario.h
 *
 *  Created on: Dec 20, 2017
 *      Author: iltis
 */
#include <string>
#include "Setup.h"
#include "S2F.h"

#ifndef MAIN_OPENVARIO_H_
#define MAIN_OPENVARIO_H_

typedef enum protocol_t  { P_OPENVARIO, P_BORGELT, P_CAMBRIDGE, P_EYE_PEYA, P_EYE_PEYI, P_AHRS_RPYL, P_AHRS_APENV1, P_GENERIC } proto_t;


class OpenVario {
public:
	OpenVario( S2F * as2f );
	virtual ~OpenVario( );
	void makeNMEA( proto_t proto, char* str, float baro, float dp, float te, float temp, float ias, float tas,
			       float mc, int bugs, float ballast, bool cruise, float alt,
				   bool validTemp=false, float ax=0, float ay=0, float az=0, float gx=0, float gy=0, float gz=0 );

	static void parseNMEA( char *str );
	static int getCheckSum(char * s);
private:
	static S2F *   _s2f;
};

#endif /* MAIN_OPENVARIO_H_ */
