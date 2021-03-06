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

#include "support/configCosmos.h"
#include "support/datalib.h"
#include "support/jsonlib.h"
#include "support/timelib.h"
#include "support/event.h"
#include "agent/scheduler.h"

using std::cout;
using std::endl;

int main(int argc, char *argv[])
{

    uint32_t flag = 0;
    string data = "";
    string condition = "";
    string name= "";
    double utc = 0.;

    string node = "";

    switch (argc)
    {
    case 8: // add command to the scheduler
        {
            node = string(argv[6]);
        }
    case 7: // set solo flag
        {
            if (atoi(argv[6]))
            {
                flag |= EVENT_FLAG_SOLO;
            }
        }
    case 6: // set repeat flag
        {
            if (atoi(argv[5]))
            {
                flag |= EVENT_FLAG_REPEAT;
            }
        }
    case 5: // set conditions
        {
            condition = argv[4];
            if (condition != "{}")
            {
                flag |= EVENT_FLAG_CONDITIONAL;
            }
        }
    case 4: // set time utc in mjd
        {
            switch (argv[3][0])
            {
            // add a few seconds to current time
            case '+':
                {
                    double seconds = atof(&argv[3][1]);
                    utc = currentmjd() + seconds / 86400.;
                    break;
                }
            default:
                // use set time
                {
                    utc = atof(argv[3]);
                    break;
                }
            }
        }
    case 3: // set command string
    {
        data = argv[2];
    }
    case 2: // set command name
    {
        name = argv[1];
        break;
    }
    default:
    {
        cout << "The command generator produces the command string to be fed into the command\n"
                "queue which is managed by agent_exec. Commands are a subset of events\n"
                "in COSMOS." << endl << endl;

        cout << "Usage" << endl << endl;
        cout << "  command_generator [options]" << endl;
        cout << "  command_generator name command [time | +sec] [condition] [repeat_flag] [node]" << endl << endl;


        cout << "Example" << endl << endl;
        cout << "  $ command_generator myCmd1 \"agent kauaicc_sim execsoh get_queue_size\" +10" << endl << endl;
        cout << "  This will run the command \"agent kauaicc_sim execsoh get_queue_size\" \n"
                "  with name 'myCmd' within 10 seconds from now." << endl << endl;

        cout << "Arguments" << endl << endl;

        cout << "  name   \t = name of the command, can be a string or combination of \n "
                "          \t   alphanumeric characters (ex: myCmd1)" << endl;
        cout << "  command\t = the actual command to be executed, if more than one word \n"
                "          \t   enclose the command string in quotes \n"
                "          \t   (ex: \"agent kauaicc_sim execsoh get_queue_size\"" << endl;
        cout << "  time   \t = optional argument to enter the desired modified julian date\n"
                "          \t   (mjd). If not entered it will use the current time. \n"
                "          \t   If '+' is used instead then the number of seconds can be\n"
                "          \t   inserted. For 10 seconds in the future use +10" << endl;
        cout << "  condition   \t = optional argument to enter a COSMOS condition\n"
                "          \t   Shoold be in JSON. Empty {} will mean no condition \n";
        cout << "  repeat_flag   \t = Optional repeat flag.\n"
                "          \t   0 or 1. \n";
        cout << "  node   \t = optional argument to add the generated command to the command\n"
                "          \t   queue on the specified node (ex: kauaicc_sim) " << endl;
        return 0;
    }
    }

    Event event;
    cout << "Command string:" << endl;
	// JIMNOTE: this could be done in the constructor

    cout << event.generator(name, data, utc, condition, flag) << endl << endl;

    if (!node.empty()) {
        cout << "Adding command/event to node " << node << endl;
        Scheduler scheduler(node);

        // Examples on how to use the scheduler class:
        //        event.name = "Track;FS701;20160930.145000;20160930.145500";
        //        event.data = "agent_tracker FS701 20160930.145000 20160930.145500";
        //        event.mjd  = cal2mjd(2016, 9, 30, 14, 50, 0, 0);
        //        DateTime time( 2016, 10, 12, 14, 50, 1 );
        //        event.mjd = time.mjd;

        //        cout << event.get_name() << endl;
        //        cout << event.get_data() << endl;
        //        cout << event.getTime() << endl;

        scheduler.addEvent(event);
        COSMOS_SLEEP(0.1);
        //        scheduler.deleteEvent(event);
        //        scheduler.getEventQueueSize();
        scheduler.getEventQueue();
    }
}
