#include "agentlib.h"

cosmosstruc *cdata;
uint8_t data[32769];
char buffer[AGENTMAXBUFFER], response[AGENTMAXBUFFER];

int main(int argc, char *argv[])
{
	beatstruc beat;
	int32_t iretn;
	uint16_t rcrc, crc, count, rcount, i, j, k;
	double cmjd, rmjd, mjd, dt1, sdt1, dt2, sdt2;
	int16_t dcrc, dcount;

	if (argc != 3)
		{
			fprintf(stderr,"Usage: echo_test node agent\n");
			exit(0);
		}

	if ((cdata=agent_setup_client(AGENT_TYPE_BROADCAST, (char *)argv[1])) == NULL)
		{
			fprintf(stderr,"Error: %d\n",AGENT_ERROR_JSON_CREATE);
			exit(AGENT_ERROR_JSON_CREATE);
		}

	if ((iretn=agent_get_server(cdata, argv[1], argv[2], 3, &beat) <= 0))
	{
		fprintf(stderr,"Error: Could not find %s:%s\n",argv[1],argv[2]);
		exit(iretn);
	}

	k = 0;
	count = 32;
	while (count < beat.bsz-40)
	{
		dt1 = sdt1 = 0.;
		dt2 = sdt2 = 0.;
		dcrc = dcount = 0;
		for (i=0; i<10; ++i)
		{
			for (j=0; j<count; ++j)
				{
					data[j] = 'a'+k%26;
					++k;
				}
			data[count] = 0;
			crc = slip_calc_crc(data,count);
			mjd = currentmjd(0);
			sprintf(buffer,"echo %.17g %4x %5u ",mjd,crc,count);
			strncpy(&buffer[strlen(buffer)],(char *)data,count+1);
			agent_send_request(cdata, beat,buffer,response,AGENTMAXBUFFER,3);
			cmjd = currentmjd(0);
			sscanf(response,"%lf %hx %hu",&rmjd,&rcrc,&rcount);
			dt1 += (rmjd - mjd);
			sdt1 += (rmjd - mjd)*(rmjd - mjd);
			dt2 += (cmjd - rmjd);
			sdt2 += (cmjd - rmjd)*(cmjd - rmjd);
			if (rcrc != crc)
				++dcrc;
			dcount += (rcount - count);
		}
		printf("%5u %f %f %f %f %.7g %4x %d\n",count,(86400.*dt1/10.),(86400.*sqrt((sdt1-dt1*dt1/10)/9)),(86400.*dt2/10.),(86400.*sqrt((sdt2-dt2*dt2/10)/9)),count/(86400.*(cmjd-mjd)/5.),dcrc, dcount);
		if (count < 65536/2)
		{
			count *= 2;
		}
		else
		{
			break;
		}
	}
}
