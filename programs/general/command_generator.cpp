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
#include "datalib.h"
#include "jsonlib.h"
#include "timelib.h"
#include "command.h"
#include "scheduler.h"

int main(int argc, char *argv[])
{
	longeventstruc com;

	com.type = EVENT_TYPE_COMMAND;
	com.flag = 0;
	com.data[0] = 0;
	com.condition[0] = 0;
	com.utc = 0;
	com.utcexec = 0.;

    //
//    std::string condition = argv[4];
//    double time_mjd = argv[3];
//    double time_seconds = argv[3][1];
//    std::string command_data = argv[2];
//    std::string command_name = argv[1];


	switch (argc)
	{
    case 6: // set repeat flag
		{
			com.flag |= EVENT_FLAG_REPEAT;
		}
    case 5: // set conditions
		{
            strcpy(com.condition, argv[4]);
			com.flag |= EVENT_FLAG_CONDITIONAL;
		}
    case 4: // set time utc in mjd
		{
			switch (argv[3][0])
			{
            // add a few seconds to current time
			case '+':
				{
					double seconds = atof(&argv[3][1]);
					com.utc = currentmjd() + seconds / 86400.;
					break;
				}
			default:
                // use set time
				{
                    com.utc = atof(argv[3]);
					break;
				}
			}
		}
    case 3: // set command data
		{
            //std::string command_data = argv[2];
            strcpy(com.data, argv[2]);
		}
    case 2: // set command name
		{
            //std::string command_name = argv[1];
			strcpy(com.name, argv[1]);
			break;
		}
	default:
		{
            printf("Usage: submit_command name command_string [time | +sec [condition [repeat_flag]]]\n");
			break;
		}
	}

//    std::string jsp;

//    json_out_commandevent(jsp, com);
//    printf("%s\n", jsp.c_str());

//    Scheduler scheduler("kauaicc_sim");
//    scheduler.addCommand(com.name,com.data, com.utc, com.condition, com.flag);


}