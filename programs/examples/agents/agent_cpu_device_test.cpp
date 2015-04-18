#include "configCosmos.h"
#include "elapsedtime.hpp"
#include "timeutils.hpp"
#include "agentlib.h"

#include <iostream>
#include <string>
using namespace std;

int myagent();
int32_t request_start(char *request, char* response_r, void *cdata); // function prototype of agent request

string agentname     = "agent_cpu_device_test";
string nodename      = "computer";

int waitsec = 5; // wait to find other agents of your 'type/name', seconds
int loopmsec = 1; // period of heartbeat
char buf4[512];

beatstruc beat_agent_cpu;
cosmosstruc *cdata; // to access the cosmos data, will change later
ElapsedTime et;

#define MAXBUFFERSIZE 256 // comm buffe for agents

int main(int argc, char *argv[])
{
	int iretn;

	cout << "Starting agent CPU Device Test" << endl;

	// Establish the command channel and heartbeat
	if (!(cdata = agent_setup_server(AGENT_TYPE_UDP,
	nodename.c_str(),
	agentname.c_str(),
	1.0,
	0,
	AGENTMAXBUFFER)))
	{
		cout << agentname << ": agent_setup_server failed (returned <"<<AGENT_ERROR_JSON_CREATE<<">)"<<endl;
		exit (AGENT_ERROR_JSON_CREATE);
	} else {
		cout<<"Starting " << agentname << " ... OK" << endl;
		//        cdata->agent[0].sub
	}

	beat_agent_cpu = agent_find_server(cdata, nodename, "agent_cpu", 10.);

	// Start our own thread
	iretn = myagent();

	return 0;
}

int myagent()
{
	cout << "agent " << agentname <<  " ...online " << endl;
	char response[300];
	int count = 1;
	int iretn;

	// Start executing the agent
	while(agent_running(cdata))
	{
		cout << "-------------------" << endl;
		cout << "Cycle: \t\t\t\t" << count << endl;

		iretn = agent_send_request(cdata,
								   beat_agent_cpu,
								   "mem",
								   response,
								   512,
								   2);
		cout << "Memory used in KB: \t\t" << response << endl;

		iretn = agent_send_request(cdata,
								   beat_agent_cpu,
								   "disk",
								   response,
								   512,
								   2);
		cout << "Disk used in KB: \t\t" << response << endl;


		iretn = agent_send_request(cdata,
								   beat_agent_cpu,
								   "load",
								   response,
								   512,
								   2);
		cout << "Load data in %: \t\t" << response << endl;

		count++;
		COSMOS_SLEEP(2.00);

	}
	return (0);

}