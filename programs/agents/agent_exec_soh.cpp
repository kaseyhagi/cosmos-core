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

#include "agentlib.h"
#include "jsonlib.h"
#include "convertlib.h"
#include "datalib.h"

#include <iostream>
#include <iomanip>

/*! \file agent_exec.cpp
* \brief Executive Agent source file
*/

//! \ingroup agents
//! \defgroup agent_exec Executive Agent program
//! This Agent manages commanding within the COSMOS system.
//! A single command queue is kept containing both time, and time and condition driven
//! commands. Commands can be added or removed from this queue, either through direct requests
//! or through command files.
//!
//! Commands are represented as a ::eventstruc. If EVENT_FLAG_CONDITIONAL is set, the condition
//! part of the ::eventstruc is evaluated as a COSMOS equation to determine whether the command
//! should be executed. Either way, commands are only executed if their time has passed. Once a
//! command has executed, it is either remove from the queue, or if EVENT_FLAG_REPEAT is set,
//! it is disabled from executing until such time as the condition goes false again, after which
//! it can once again execute.
//!
//! Any execution of a command is reflected in two log files, one of which tracks the results of the
//! command, and the other of which logs the actual ::eventstruc for the command, with the utcexec field
//! set to the actual time of execution.
//!
//! Usage: agent_exec node_name
//#ifdef _MSC_BUILD
//#include "dirent/dirent.h"
//#else
//#include <dirent.h>
//#endif
#include <list>
#include <fstream>
#include <sstream>

cosmosstruc* cdata;

// Exuctive specfic declarations
#include "agent_exec.h"

#define MAXCOMMANDWORD 20

string incoming_dir;
string outgoing_dir;
string temp_dir;

string nodename;
DIR *dir = NULL;
struct dirent *dir_entry = NULL;
double logdate_exec=0.;
double newlogstride_exec = 900. / 86400.;
double logstride_exec = 0.;
bool queue_changed = false;

int32_t request_get_queue_size(char *request, char* response, void *cdata);
int32_t request_get_queue_entry(char *request, char* response, void *cdata);
int32_t request_del_queue_entry(char *request, char* response, void *cdata);
int32_t request_add_queue_entry(char *request, char* response, void *cdata);
int32_t request_run(char *request, char* response, void *cdata);
int32_t request_soh(char *request, char* response, void *cdata);
int32_t request_reopen_exec(char* request, char* output, void *cdata);
int32_t request_set_logstride_exec(char* request, char* output, void *cdata);

void collect_data_loop();
command_queue cmd_queue;

// SOH specific declarations
int32_t request_reopen_soh(char* request, char* output, void *cdata);
int32_t request_set_logperiod(char* request, char* output, void *cdata);
int32_t request_set_logstring(char* request, char* output, void *cdata);
int32_t request_get_logstring(char* request, char* output, void *cdata);
int32_t request_set_logstride_soh(char* request, char* output, void *cdata);

string jjstring;
string myjstring;

int ntype = SOCKET_TYPE_UDP;
int waitsec = 5;

void collect_data_loop();
thread cdthread;

int myagent();

string logstring;
vector<jsonentry*> logtable;
double logdate_soh=0.;
int32_t newlogperiod = 10, logperiod = 0;
double newlogstride_soh = 900. / 86400.;
double logstride_soh = 0.;

vector<shorteventstruc> eventdict;
vector<shorteventstruc> events;

int pid;
int state = 0;
double cmjd;

beatstruc iscbeat;
string node = "hiakasat";
char response[300];

int main(int argc, char *argv[])
{
	int sleept;
	double lmjd, dmjd;
	double nextmjd;

	cout<<"Starting the executive/soh agent...";
	int32_t iretn;
	int ntype = SOCKET_TYPE_UDP;

	// Set node name to first argument
	if (argc!=2)
	{
		cout<<"Usage: agent_exec_soh node"<<endl;
		exit(1);
	}
	nodename = argv[1];

	// Establish the command channel and heartbeat
	cdata = agent_setup_server(ntype, (char *)nodename.c_str(), (char *)"execsoh", 1., 0, AGENTMAXBUFFER);
	if(cdata == NULL)
	{
		cout<<"agent_exec: agent_setup_server failed (returned <"<<AGENT_ERROR_JSON_CREATE<<">)"<<endl;
		exit (AGENT_ERROR_JSON_CREATE);
	}
	cdata[0].node.utc = 0.;
	cdata[0].agent[0].aprd = .5;
	json_clone(cdata);

	cout<<"  started."<<endl;

	// Establish Executive functions

	// Set the incoming, outgoing, and temp directories
	incoming_dir = data_base_path(nodename, "incoming", "exec") + "/";
	if (incoming_dir.empty())
	{
		cout<<"unable to create directory: <"<<(nodename+"/incoming")+"/exec"<<"> ... exiting."<<endl;
		exit(1);
	}
	outgoing_dir = data_base_path(nodename, "outgoing", "exec") + "/";
	if (outgoing_dir.empty())
	{
		cout<<"unable to create directory: <"<<(nodename+"/outgoing")+"/exec"<<"> ... exiting."<<endl;
		exit(1);
	}
	outgoing_dir = data_base_path(nodename, "outgoing", "exec") + "/";
	if (outgoing_dir.empty())
	{
		cout<<"unable to create directory: <"<<(nodename+"/outgoing")+"/exec"<<"> ... exiting."<<endl;
		exit(1);
	}
	temp_dir = data_base_path(nodename, "temp", "exec") + "/";
	if (temp_dir.empty())
	{
		cout<<"unable to create directory: <"<<(nodename+"/temp")+"/exec"<<"> ... exiting."<<endl;
		exit(1);
	}

	// Add agent request functions
	if ((iretn=agent_add_request(cdata, (char *)"get_queue_size",request_get_queue_size,"", "returns the current size of the command queue")))
		exit (iretn);
	if ((iretn=agent_add_request(cdata, (char *)"del_queue_entry",request_del_queue_entry,"entry #","deletes the specified command queue entry")))
		exit (iretn);
	if ((iretn=agent_add_request(cdata, (char *)"get_queue_entry",request_get_queue_entry,"[ entry # ]","returns the requested command queue entry (or all if none specified)")))
		exit (iretn);
	if ((iretn=agent_add_request(cdata, (char *)"add_queue_entry",request_add_queue_entry,"{\"event_name\":\"\"}{\"event_utc\":0}{\"event_utcexec\":0}{\"event_flag\":0}{\"event_type\":0}{\"event_data\":\"\"}{\"event_condition\":\"\"}","adds the specified command queue entry")))
		exit (iretn);
	if ((iretn=agent_add_request(cdata, (char *)"run",request_run,"","run the requested command")))
		exit (iretn);
	//		if ((iretn=agent_add_request(cdata, (char *)"this_is_a_super_long_ass_name",request_run)))
	//				exit (iretn);
	if ((iretn=agent_add_request(cdata, (char *)"reopen_exec",request_reopen_exec)))
		exit (iretn);
	if ((iretn=agent_add_request(cdata, (char *)"set_logstride_exec",request_set_logstride_exec)))
		exit (iretn);

	// Reload existing queue
	string infilepath = temp_dir + ".queue";
	ifstream infile(infilepath.c_str());
	if(!infile.is_open())
	{
		cout<<"unable to read file <"<<infilepath<<">"<<endl;
	}
	else
	{

		//file is open for reading commands
		string line;
		command cmd;

		while(getline(infile,line))
		{
			cmd.set_command(line);

			cout<<cmd;

			if(cmd.is_command())
			{
				cmd_queue.add_command(cmd);
				printf("Loaded command: %s\n", line.c_str());
			}
			else
			{
				cout<<"Not a command!"<<endl;
			}
		}
		infile.close();
	}

	// Establish SOH functions

	if ((iretn=agent_add_request(cdata, (char *)"reopen_soh",request_reopen_soh)))
		exit (iretn);
	if ((iretn=agent_add_request(cdata, (char *)"set_logperiod",request_set_logperiod)))
		exit (iretn);
	if ((iretn=agent_add_request(cdata, (char *)"set_logstring",request_set_logstring)))
		exit (iretn);
	if ((iretn=agent_add_request(cdata, (char *)"get_logstring",request_get_logstring)))
		exit (iretn);
	if ((iretn=agent_add_request(cdata, (char *)"set_logstride_soh",request_set_logstride_soh)))
		exit (iretn);

	// Create default logstring
	logstring = json_list_of_soh(cdata);
	printf("logstring: %s\n", logstring.c_str());
	json_table_of_list(logtable, logstring.c_str(), cdata);
	//	agent_set_sohstring(cdata, logstring.c_str());

	load_dictionary(eventdict, cdata, (char *)"events.dict");

	// Start thread to collect SOH data
	cdthread = thread(collect_data_loop);

	// Start performing the body of the agent
	nextmjd = currentmjd();
	lmjd = currentmjd();
	while(agent_running(cdata))
	{
		nextmjd += cdata[0].agent[0].aprd/86400.;
		dmjd = (cmjd-lmjd)*86400.;
		cdata[0].node.utc = cmjd = currentmjd();

		// Check if the SOH logperiod has changed
		if (newlogperiod != logperiod )
		{
			logperiod = newlogperiod;
			logdate_soh = cdata[0].node.utc;
			log_move(cdata[0].node.name, "soh");
		}


		// Check if either of the logstride have changed
		if (newlogstride_exec != logstride_exec )
		{
			logstride_exec = newlogstride_exec;
			logdate_exec = currentmjd(0.);
			log_move(nodename, "exec");
		}

		if (newlogstride_soh != logstride_soh )
		{
			logstride_soh = newlogstride_soh;
			logdate_soh = currentmjd(0.);
			log_move(nodename, "soh");
		}

		// Check if either of the logstride have expired
		if (floor(cmjd/logstride_exec)*logstride_exec > logdate_exec)
		{
			logdate_exec = floor(cmjd/logstride_exec)*logstride_exec;
			log_move(nodename, "exec");
		}

		if (floor(cmjd/logstride_soh)*logstride_soh > logdate_soh)
		{
			logdate_soh = floor(cmjd/logstride_soh)*logstride_soh;
			log_move(nodename, "soh");
		}

		// Perform SOH specific functions
		if (cdata[0].node.utc != 0.)
		{
			loc_update(&cdata[0].node.loc);
			update_target(cdata);
			agent_post(cdata, AGENT_MESSAGE_SOH, json_of_table(myjstring, logtable, cdata));
			calc_events(eventdict, cdata, events);
			for (uint32_t k=0; k<events.size(); ++k)
			{
				memcpy(&cdata[0].event[0].s,&events[k],sizeof(shorteventstruc));
				strcpy(cdata[0].event[0].l.condition,cdata[0].emap[events[k].handle.hash][events[k].handle.index].text);
				log_write(cdata[0].node.name,DATA_LOG_TYPE_EVENT,logdate_soh, json_of_event(jjstring,cdata));
			}
		}

		// Check if SOH logperiod has expired
		if (dmjd-logperiod > -logperiod/20.)
		{
			lmjd = cmjd;
			if (cdata[0].node.utc != 0. && logstring.size())
			{
				log_write(cdata[0].node.name,DATA_LOG_TYPE_SOH, logdate_soh, json_of_table(jjstring, logtable, cdata));
			}
		}

		// Perform Executive specific functions
		cmd_queue.load_commands();
		cmd_queue.run_commands();
		cmd_queue.save_commands();

		sleept = (int)((nextmjd-currentmjd())*86400000000.);
		if (sleept < 0) sleept = 0;
		COSMOS_USLEEP(sleept);
	}

	agent_shutdown_server(cdata);
	cdthread.join();
}

// Executive specific requests
int32_t request_set_logstride_exec(char* request, char* output, void *cdata)
{
	sscanf(request,"set_logstride_exec %lf",&newlogstride_exec);
	return 0;
}

int32_t request_reopen_exec(char* request, char* output, void *cdata)
{
	logdate_exec = ((cosmosstruc *)cdata)->node.loc.utc;
	log_move(((cosmosstruc *)cdata)->node.name, "exec");
	return 0;
}

int32_t request_get_queue_size(char *request, char* response, void *cdata)
{
	sprintf(response,"%" PRIu32 "", cmd_queue.get_size());
	return 0;
}

int32_t request_get_queue_entry(char *request, char* response, void *cdata)
{
	ostringstream ss;

	if(cmd_queue.get_size()==0)
		ss << "the command queue is empty";
	else
	{
		int j;
		int32_t iretn = sscanf(request,"get_queue_entry %d",&j);

		// if valid index then return command
		if (iretn == 1)
			if(j >= 0 && j < (int)cmd_queue.get_size() )
				ss << cmd_queue.get_command(j);
			else
				ss << "<" << j << "> is not a valid command queue index (current range between 0 and " << cmd_queue.get_size()-1 << ")";

		// if no index given, return the entire queue
		else if (iretn ==  -1)
			for(unsigned long int i = 0; i < cmd_queue.get_size(); ++i)
				ss << cmd_queue.get_command(i) << endl;

		// if the user supplied something that couldn't be turned into an integer
		else if (iretn == 0)
			ss << "Usage:\tget_queue_entry [ index ]\t";
	}

	strcpy(response, ss.str().c_str());
	return 0;
}

// Delete Queue Entry - by date and contents
int32_t request_del_queue_entry(char *request, char* response, void *cdata)
{
	command cmd;
	string line(request);

	// remove "del_queue_entry " from request string
	line.erase(0, 16);

	cmd.set_command(line);

	//delete command
	int n = cmd_queue.del_command(cmd);

	sprintf(response,"%d commands deleted from the queue",n);

	return 0;
}

// Add Queue Entry
int32_t request_add_queue_entry(char *request, char* response, void *cdata)
{
	command cmd;
	string line(request);

	// remove "add_queue_entry " from request string
	line.erase(0, 16);

	cmd.set_command(line);

	// add command
	if(cmd.is_command())
		cmd_queue.add_command(cmd);

	strcpy(response, line.c_str());
	return 0;
}

int32_t request_run(char *request, char* response, void *cdata)
{
	int i;
	int32_t iretn = 0;
	FILE *pd;
	bool flag;

	// Run Program
	flag = false;

	for (i=0; i<AGENTMAXBUFFER-1; i++)
	{
		if (flag)
		{
			if (request[i] != ' ')
				break;
		}
		else
		{
			if (request[i] == ' ')
				flag = true;
		}
	}

	if (i == AGENTMAXBUFFER-1)
	{
		sprintf(response,"unmatched");
	}
	else
	{
#ifdef COSMOS_WIN_BUILD_MSVC
		if ((pd=_popen(&request[i], "r")) != NULL)
#else
		if ((pd=popen(&request[i],"r")) != NULL)
#endif
		{
			iretn = fread(response,1,AGENTMAXBUFFER-1,pd);
			response[iretn] = 0;

			iretn = 0;
#ifdef COSMOS_WIN_BUILD_MSVC
			_pclose(pd);
#else
			pclose(pd); // close process
#endif
		}
		else
		{
			response[0] = 0;
			iretn = 0;
		}
	}

	return (iretn);
}

// SOH specific requests
int32_t request_reopen_soh(char* request, char* output, void *cdata)
{
	logdate_soh = ((cosmosstruc *)cdata)->node.loc.utc;
	log_move(((cosmosstruc *)cdata)->node.name, "soh");
	return 0;
}

int32_t request_set_logperiod(char* request, char* output, void *cdata)
{
	sscanf(request,"set_logperiod %d",&newlogperiod);
	return 0;
}

int32_t request_set_logstring(char* request, char* output, void *cdata)
{
	logstring = &request[strlen("set_logstring")+1];
	json_table_of_list(logtable, logstring.c_str(), (cosmosstruc*)cdata);
	return 0;
}

int32_t request_get_logstring(char* request, char* output, void *cdata)
{
	strcpy(output, logstring.c_str());
	return 0;
}

int32_t request_set_logstride_soh(char* request, char* output, void *cdata)
{
	sscanf(request,"set_logstride_soh %lf",&newlogstride_soh);
	return 0;
}

void collect_data_loop()
{
	int nbytes;
	string message;
	pollstruc meta;

	while (agent_running(cdata))
	{
		// Collect new data
		if((nbytes=agent_poll(cdata, meta, message, AGENT_MESSAGE_BEAT, 0)))
		{
			if (json_convert_string(json_extract_namedobject(message.c_str(), "agent_node")) != cdata[0].node.name)
			{
				continue;
			}
			cdata[1].node = cdata[0].node;
			cdata[1].device = cdata[0].device;
			json_parse(message,&cdata[1]);
			cdata[0].node  = cdata[1].node ;
			cdata[0].device  = cdata[1].device ;
			loc_update(&cdata[0].node.loc);
			cdata[0].node.utc = currentmjd(0.);
		}
	}
	return;
}

// Prints the command information stored in local the copy of cdata[0].event[0].l
void print_command()
{
	string jsp;

	json_out(jsp,(char*)"event_utc",cdata);
	json_out(jsp,(char*)"event_utcexec",cdata);
	json_out(jsp,(char*)"event_name",cdata);
	json_out(jsp,(char*)"event_type",cdata);
	json_out(jsp,(char*)"event_flag",cdata);
	json_out(jsp,(char*)"event_data",cdata);
	json_out(jsp,(char*)"event_condition",cdata);
	cout<<"<"<<jsp<<">"<<endl;

	return;
}



// *************************************************************************
// Class: command
// *************************************************************************

// Copies the current command object to the output stream using JSON format
ostream& operator<<(ostream& out, const command& cmd)
{
	out	<< setprecision(15) <<"{\"event_utc\":"<< cmd.utc
		<< "}{\"event_utcexec\":" << cmd.utcexec
		<< "}{\"event_name\":\"" << cmd.name
		<< "\"}{\"event_type\":" << cmd.type
		<< "}{\"event_flag\":" << cmd.flag
		<< "}{\"event_data\":\"" << cmd.data
		<< "\"}{\"event_condition\":\"" << cmd.condition
		<< "\"}";
	return out;
}

// Equality Operator for command objects
bool operator==(const command& cmd1, const command& cmd2)
{
	return (	cmd1.name==cmd2.name &&
				cmd1.utc==cmd2.utc &&
				cmd1.utcexec==cmd2.utcexec &&
				cmd1.type==cmd2.type &&
				cmd1.flag==cmd2.flag &&
				cmd1.data==cmd2.data &&
				cmd1.condition==cmd2.condition);
}


// Default Constructor for command objects
command::command() : utc(0), utcexec(0), name(""), type(0), flag(0), data(""), condition(""), already_ran(false)
{
}

// Copies the command information stored in the local copy
// cdata[0].event[0].l into the current command object
void command::set_command(string line)
{
	json_clear_cosmosstruc(JSON_GROUP_EVENT, &cdata[1]);
	json_parse(line, &cdata[1]);
	utc = cdata[1].event[0].l.utc;
	utcexec = cdata[1].event[0].l.utcexec;
	name = cdata[1].event[0].l.name;
	type = cdata[1].event[0].l.type;
	flag = cdata[1].event[0].l.flag;
	data = cdata[1].event[0].l.data;
	condition = cdata[1].event[0].l.condition;
}

string command::get_json()
{
	string jsp;

	longeventstruc event;

	event.utc = utc;
	event.utcexec = utcexec;
	strcpy(event.name, name.c_str());
	event.type = type;
	event.flag = flag;
	strcpy(event.data, data.c_str());
	strcpy(event.condition, condition.c_str());

	json_out_commandevent(jsp, event);
	return jsp;
}



// *************************************************************************
// Class: command_queue
// *************************************************************************

// Copies the current command_queue object to the output stream using JSON format
ostream& operator<<(ostream& out, command_queue& cmdq)
{
	for(list<command>::iterator ii = cmdq.commands.begin(); ii != cmdq.commands.end(); ++ii)
		out << *ii << endl;
	return out;
}


// Executes a command using fork().  For each command run, the time of
// execution (utcexec) is set, the flag EVENT_FLAG_ACTUAL is set to true,
// and this updated command information is logged to the OUTPUT directory.
void run_command(command& cmd)
{
	queue_changed = true;

	// set time executed & actual flag
	cmd.set_utcexec();
	cmd.set_actual();

	// execute command
#if defined(COSMOS_WIN_OS)
	char command_line[100];
	strcpy(command_line, cmd.get_data());

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);
	ZeroMemory( &pi, sizeof(pi) );

	if (CreateProcess(NULL, command_line, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
	{
		//		int32_t pid = pi.dwProcessId;
		CloseHandle( pi.hProcess );
		CloseHandle( pi.hThread );
	}
#else
	signal(SIGCHLD, SIG_IGN);
	int32_t pid = fork();
	switch(pid)
	{
	case -1:
		break;
	case 0:
		char *words[MAXCOMMANDWORD];
		int devn;
		string_parse(cmd.get_data(),words,MAXCOMMANDWORD);
		string outpath = data_type_path(nodename, "temp", "exec", logdate_exec, "out");
		if (outpath.empty())
		{
			devn = open("/dev/null",O_RDWR);
		}
		else
		{
			devn = open(outpath.c_str(), O_CREAT|O_WRONLY|O_APPEND, 00666);
		}
		dup2(devn, STDIN_FILENO);
		dup2(devn, STDOUT_FILENO);
		dup2(devn, STDERR_FILENO);
		close(devn);
		execvp(words[0],&(words[0]));
		fflush(stdout);
		exit (0);
		break;
	}
#endif

	// log to outfile
	//	outfile << cmd <<endl;
	//	outfile.close();
	// log to event file
	log_write(nodename, "exec", logdate_exec, "event", cmd.get_json().c_str());
}


// Manages the logic of when to run commands in the command queue.
void command_queue::run_commands()
{
	for(list<command>::iterator ii = commands.begin(); ii != commands.end(); ++ii)
	{
		if (ii->is_ready())
		{
			if (ii->is_conditional())
			{
				if(ii->condition_true())
				{
					if(ii->is_repeat())
					{
						if(!ii->already_ran)	{
							run_command(*ii);
							ii->already_ran = true;
						}
					}
					else // non-repeatable
					{
						run_command(*ii);
						commands.erase(ii--);
					}
				} // condition is false
				else
				{
					ii->already_ran = false;
				}
			}
			else  // non-conditional
			{
				run_command(*ii);
				commands.erase(ii--);
			}
		}
	}
	return;
}

// Saves commands to .queue file located in the temp directory
// Commands are taken from the global command queue
// Command queue is sorted by utc after loading
void command_queue::save_commands()
{
	if (!queue_changed)
	{
		return;
	}
	queue_changed = false;

	// Open the outgoing file
	FILE *fd = fopen((temp_dir+".queue").c_str(), "w");
	if (fd != NULL)
	{
		for (command cmd: commands)
		{
			fprintf(fd, "%s\n", cmd.get_json().c_str());
		}
		fclose(fd);
	}
}

// Loads new commands from *.command files located in the incoming directory
// Commands are loaded into the global command_queue object (cmd_queue),
// *.command files are removed, and the command list is sorted by utc.
void command_queue::load_commands()
{

	// open the incoming directory
	if ((dir = opendir((char *)incoming_dir.c_str())) == NULL)
	{
		cout<<"error: unable to open node's incoming directory <"<<incoming_dir<<"> not found"<<endl;
		agent_shutdown_server(cdata);
		exit(1);
	}

	// cycle through all the file names in the incoming directory
	while((dir_entry = readdir(dir)) != NULL)
	{
		string filename = dir_entry->d_name;

		if (filename.find(".command") != string::npos)
		{

			string infilepath = incoming_dir + filename;
			ifstream infile(infilepath.c_str());
			if(!infile.is_open())
			{
				cout<<"unable to read file <"<<infilepath<<">"<<endl;
				continue;
			}

			//file is open for reading commands
			string line;
			command cmd;

			while(getline(infile,line))
			{
				//clear cdata[0].event[0].l, parse line into cdata[0].event[0].l, set command object, and add to command queue
				//				json_parse("{\"event_name\":\"\"}{\"event_utc\":0}{\"event_utcexec\":0}{\"event_flag\":0}{\"event_type\":0}{\"event_data\":\"\"}{\"event_condition\":\"\"}", cdata);
				//				cout<<"<"<<line.c_str()<<">"<<endl;
				//				cout<<"Returned "<<ireturn<<" And the command is: "<<endl;

				cmd.set_command(line);

				cout<<cmd;

				if(cmd.is_command())
					cmd_queue.add_command(cmd);
				else
					cout<<"Not a command!"<<endl;
			}
			infile.close();

			//remove the .command file from incoming directory
			if(remove(infilepath.c_str()))	{
				cout<<"unable to delete file <"<<filename<<">"<<endl;
				continue;
			}

			cout<<"The size of the command queue is: "<<cmd_queue.get_size()<<endl;
		}
	}

	cmd_queue.sort();
	closedir(dir);

	return;
}

// Remove command object from the command queue, uses command == operator)
int command_queue::del_command(command& c)
{
	int n = 0;
	for(list<command>::iterator ii = commands.begin(); ii != commands.end(); ++ii)
	{
		if(c==*ii)
		{
			commands.erase(ii--);
			n++;
		}
	}
	queue_changed = true;
	return n;
}

void command_queue::add_command(command& c)
{
	commands.push_back(c);
	queue_changed = true;
}