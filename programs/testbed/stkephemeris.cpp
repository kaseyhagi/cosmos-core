/********************************************************************
* Copyright (C) 2015 by Interstel Technologies, Inc.
*   and Hawaii Space Flight Laboratory.
*
* This file is part of the COSMOS/core that is the central
* module for COSMOS. For more information on COSMOS go to
* <http://cosmos-project.com>
*
* The COSMOS/core software is licenced under the
* GNU Lesser General Public License (LGPL) version 3 licence.
*
* You should have received a copy of the
* GNU Lesser General Public License
* If not, go to <http://www.gnu.org/licenses/>
*
* COSMOS/core is free software: you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License
* as published by the Free Software Foundation, either version 3 of
* the License, or (at your option) any later version.
*
* COSMOS/core is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* Refer to the "licences" folder for further information on the
* condititons and terms to use this software.
********************************************************************/

#include "configCosmos.h"
#include "agent/agent.h"
#include "jsonlib.h"
#include "physics/physicslib.h"
#include "math/mathlib.h"
#include "jsonlib.h"
#include "agentlib.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
// #include <unistd.h>
//#include <sys/types.h>
#include <sys/stat.h>

svector azel;
std::vector<nodestruc> track;
stkstruc stk;
char fname[200];
char ctemp[AGENTMAXBUFFER];
std::string buf;
typedef struct
	{
	char code[10];
	char flags[7];
	} alert_type;

alert_type alerts[22] = {
	{"AOS","G11002"},
	{"AOS+5","G41000"},
	{"AOS+10","G31000"},
	{"MAX","G21000"},
	{"LOS+10","G31000"},
	{"LOS+5","G41320"},
	{"LOS","G11320"},
	{"TIV","T11216"},
	{"TOV","T21006"},
	{"NPOLE","O10000"},
	{"SPOLE","O10000"},
	{"N60D","O12000"},
	{"N30D","O12000"},
	{"S30D","O12000"},
	{"S60D","O12000"},
	{"S30A","O12000"},
	{"N30A","O12000"},
	{"N60A","O12000"},
	{"EQA","O12000"},
	{"EQD","O10000"},
	{"UMB_IN","U11321"},
	{"UMB_OUT","U11321"}};

std::string output;
char date[30];
FILE *eout, *fout;

void alertfor(char *string1, char *string2);

int main(int argc, char *argv[])
{
char ts[20];
alert_type talert;
double fyear;
beatstruc imubeat;
double mjdnow, lastlat, lastsunradiance, fd, lastgsel[MAXTRACK];
int i, j, gsup[MAXTRACK];
int year, month, day, hour, minute, second;
struct stat sbuf;
std::string jstring;

cosmosAgent agent(NetworkType::UDP, argv[1]);
if (argc == 3)
	{
	mjdnow = atof(argv[2]);
	}
else
	{
    agent.get_server(agent.cinfo->pdata.node.name,(char *)"engine",5,&imubeat);
    agent.send_request(imubeat, "statevec", buf, 5);
    json_parse(buf, agent.cinfo->meta, agent.cinfo->pdata);

    mjdnow = agent.cinfo->pdata.node.loc.utc;
	fyear = mjd2year(mjdnow);
	year = (int)fyear;
	day = (int)(365.26 * (fyear-year)) + 1;
	sprintf(fname,"data/%4d/%03d",year,day);
#if defined(COSMOS_WIN_OS)
	mkdir(fname);
#else
	mkdir(fname,00775);
#endif
	sprintf(fname,"data/%4d/%03d/orbitalevents",year,day);
	if (stat(fname,&sbuf) == -1)
		{
		// Move old orbitalevents to directory
		rename("orbitalevents",fname);
		}
	}

if ((eout=fopen("groundstation.ini","r")) == NULL)
	return (NODE_ERROR_GROUNDSTATION);

fgets(ctemp,35000,eout);
buf = ctemp;
json_parse(buf, agent.cinfo->meta, agent.cinfo->pdata);
fclose(eout);

if ((eout=fopen("flags.ini","r")) != NULL)
	{
    while (fgets(ctemp,50,eout) != NULL)
		{
        sscanf(ctemp,"%s %s",talert.code,talert.flags);
		for (i=0; i<22; i++)
			{
			if (!strcmp(talert.code,alerts[i].code))
				alerts[i] = talert;
			}
		}
	fclose(eout);
	}


orbit_init(0, 5., 0., "stk_data_icrs.txt", agent.cinfo->pdata);
mjdnow = agent.cinfo->pdata.node.loc.utc;
lastlat = agent.cinfo->pdata.node.loc.pos.geod.s.lat;
for (j=0; j<agent.cinfo->pdata.node.target_cnt; j++)
	{
    azel = groundstation(agent.cinfo->pdata.node.loc,track[j].loc);
	lastgsel[j] = azel.phi;
	gsup[j] = 0;
	}
lastsunradiance = agent.cinfo->pdata.node.loc.pos.sunradiance;
fout = fopen("ephemeris","w");
eout = fopen("orbitalevents","w");
for (i=0; i<9331; i++)
	{
	mjdnow += 10./86400.;
    orbit_propagate(agent.cinfo->pdata, mjdnow);
    output = json_of_ephemeris(jstring, agent.cinfo->meta, agent.cinfo->pdata);
	fprintf(fout,"%s\n",output.c_str());
	fflush(fout);
//	mjd2cal(mjdnow,&year,&month,&day,&fd,&iretn);
	mjd2ymd(mjdnow, year, month, fd);
	day = (int32_t)fd;
	fd -= day;
	hour = (int)(24. * fd);
	minute = (int)(1440. * fd - hour * 60.);
	second = (int)(86400. * fd - hour * 3600. - minute * 60.);
	sprintf(date,"%4d-%02d-%02d,%02d:%02d:%02d",year,month,day,hour,minute,second);
    if (lastlat <= RADOF(-60.) && agent.cinfo->pdata.node.loc.pos.geod.s.lat >= RADOF(-60.))
		alertfor((char *)"S60A",(char *)"");
    if (lastlat <= RADOF(-30.) && agent.cinfo->pdata.node.loc.pos.geod.s.lat >= RADOF(-30.))
		alertfor((char *)"S30A",(char *)"");
    if (lastlat <= 0. && agent.cinfo->pdata.node.loc.pos.geod.s.lat >= 0.)
		alertfor((char *)"EQA",(char *)"");
    if (lastlat <= RADOF(30.) && agent.cinfo->pdata.node.loc.pos.geod.s.lat >= RADOF(30.))
		alertfor((char *)"N30A",(char *)"");
    if (lastlat <= RADOF(60.) && agent.cinfo->pdata.node.loc.pos.geod.s.lat >= RADOF(60.))
		alertfor((char *)"N60A",(char *)"");
    if (lastlat >= RADOF(-60.) && agent.cinfo->pdata.node.loc.pos.geod.s.lat <= RADOF(-60.))
		alertfor((char *)"S60D",(char *)"");
    if (lastlat >= RADOF(-30.) && agent.cinfo->pdata.node.loc.pos.geod.s.lat <= RADOF(-30.))
		alertfor((char *)"S30D",(char *)"");
    if (lastlat >= 0. && agent.cinfo->pdata.node.loc.pos.geod.s.lat <= 0.)
		alertfor((char *)"EQD",(char *)"");
    if (lastlat >= RADOF(30.) && agent.cinfo->pdata.node.loc.pos.geod.s.lat <= RADOF(30.))
		alertfor((char *)"N30D",(char *)"");
    if (lastlat >= RADOF(60.) && agent.cinfo->pdata.node.loc.pos.geod.s.lat <= RADOF(60.))
		alertfor((char *)"N60D",(char *)"");

    if (lastsunradiance == 0. && agent.cinfo->pdata.node.loc.pos.sunradiance > 0.)
		{
		alertfor((char *)"UMB_OUT",(char *)"");
		}
    if (lastsunradiance > 0. && agent.cinfo->pdata.node.loc.pos.sunradiance == 0.)
		{
		alertfor((char *)"UMB_IN",(char *)"");
		}
    lastlat = agent.cinfo->pdata.node.loc.pos.geod.s.lat;
    lastsunradiance = agent.cinfo->pdata.node.loc.pos.sunradiance;
    for (j=0; j<agent.cinfo->pdata.node.target_cnt; j++)
		{
        azel = groundstation(agent.cinfo->pdata.node.loc,track[j].loc);
		if (azel.phi <= lastgsel[j] && azel.phi > 0.)
			{
			if (gsup[j] > 0.)
				{
				sprintf(ts,"_%3s_%03.0f",track[j].name,DEGOF(azel.phi));
				alertfor((char *)"MAX",ts);
				}
			gsup[j] = -1;
			}
		else
			gsup[j] = 1;
		sprintf(ts,"_%3s",track[j].name);
		if (azel.phi < RADOF(10.) && lastgsel[j] >= RADOF(10.))
			{
			alertfor((char *)"LOS+10",ts);
			}
		if (azel.phi < RADOF(5.) && lastgsel[j] >= RADOF(5.))
			{
			alertfor((char *)"LOS+5",ts);
			}
		if (azel.phi < 0. && lastgsel[j] >= 0.)
			{
			alertfor((char *)"LOS",ts);
			}
		if (azel.phi >= 0. && lastgsel[j] < 0.)
			{
			alertfor((char *)"AOS",ts);
			}
		if (azel.phi >= RADOF(5.) && lastgsel[j] < RADOF(5.))
			{
			alertfor((char *)"AOS+5",ts);
			}
		if (azel.phi >= RADOF(10.) && lastgsel[j] < RADOF(10.))
			{
			alertfor((char *)"AOS+10",ts);
			}
		lastgsel[j] = azel.phi;
		}
	}
fclose(fout);
fclose(eout);
}

void alertfor(char *string1, char *string2)
{
int i;

for (i=0; i<22; i++)
	{
	if (!strcmp(string1,alerts[i].code))
		break;
	}

if (i < 22)
	{
	fprintf(eout,"%s,%s,%s%s\n",date,alerts[i].flags,string1,string2);
	}
}
