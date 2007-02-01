/*
	KSysGuard, the KDE System Guard
	
	Copyright (c) 2007 Greg Martyn <greg.martyn@gmail.com>
	
	This program is free software; you can redistribute it and/or
	modify it under the terms of version 2 of the GNU General Public
	License as published by the Free Software Foundation.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "Command.h"
#include "ksysguardd.h"
#include "softraid.h"

#include <string.h> /* for strlen, strcat and strcmp */
#include <stdio.h> /* for sprintf */
#include <sys/types.h> /* for open */
#include <sys/stat.h> /* for open */
#include <fcntl.h> /* for open */
#include <unistd.h> /* for read, close, exec, fork */
#include <stdlib.h> /* for exit */
#include <sys/wait.h> /* for wait :) */
#include <stdbool.h> /* for bool */
#include "ccont.h" /* for CONTAINER */

#define MDSTATBUFSIZE (1 * 1024)
#define MDADMSTATBUFSIZE (2 * 1024)
#define ARRAYNAMELEN 32

static struct SensorModul* StatSM;

static CONTAINER ArrayInfos = 0;
char mdstatBuf[ MDSTATBUFSIZE ];	/* Buffer for /proc/mdstat */

typedef struct {
	bool Alive;

	/* from /proc/mdstat */
	char ArrayName[ ARRAYNAMELEN ];
	bool NumBlocksIsAlive;
	bool NumBlocksIsRegistered;
	int NumBlocks;

	/* from /sbin/mdadm --detail /dev/ArrayName */
	bool ArraySizeIsAlive;
	bool ArraySizeIsRegistered;
	int ArraySizeBlocks;
	float ArraySizeMiB;
	float ArraySizeMB;

	bool UsedDeviceSizeIsAlive;
	bool UsedDeviceSizeIsRegistered;
	int UsedDeviceSizeBlocks;
	float UsedDeviceSizeMiB;
	float UsedDeviceSizeMB;
	
	bool NumRaidDevicesIsAlive;
	bool NumRaidDevicesIsRegistered;
	int NumRaidDevices;
	
	bool TotalDevicesIsAlive;
	bool TotalDevicesIsRegistered;
	int TotalDevices;

	bool PreferredMinorIsAlive;
	bool PreferredMinorIsRegistered;
	int PreferredMinor;
	
	bool ActiveDevicesIsAlive;
	bool ActiveDevicesIsRegistered;
	int ActiveDevices;
	
	bool WorkingDevicesIsAlive;
	bool WorkingDevicesIsRegistered;
	int WorkingDevices;
	
	bool FailedDevicesIsAlive;
	bool FailedDevicesIsRegistered;
	int FailedDevices;
	
	bool SpareDevicesIsAlive;
	bool SpareDevicesIsRegistered;
	int SpareDevices;
} ArrayInfo;

void KillArrayInfo(ArrayInfo* MyArray) {
	MyArray->NumBlocksIsAlive = false;

	MyArray->ArraySizeIsAlive = false;
	MyArray->UsedDeviceSizeIsAlive = false;
	MyArray->NumRaidDevicesIsAlive = false;
	MyArray->TotalDevicesIsAlive = false;
	MyArray->PreferredMinor = false;
	MyArray->ActiveDevices = false;
	MyArray->WorkingDevices = false;
	MyArray->FailedDevices = false;
	MyArray->SpareDevices = false;
}

static int ArrayInfoEqual( void* s1, void* s2 )
{
	/* Returns 0 if both ArrayInfos have the same name.  */
	return strcmp( ((ArrayInfo*)s1)->ArrayName, ((ArrayInfo*)s2)->ArrayName );
}

void printArrayAttribute( const char* cmd ) {
	INDEX idx;
	ArrayInfo key;
	ArrayInfo* foundArray;
	char Attribute[32];

	if ( sscanf(cmd, "SoftRaid/%[^/]/%s", key.ArrayName, Attribute) == 2 ) {
		if ( ( idx = search_ctnr( ArrayInfos, ArrayInfoEqual, &key ) ) == 0 ) {
			foundArray = get_ctnr( ArrayInfos, idx );

			if ( strcmp( Attribute, "NumBlocks" ) == 0 )
				fprintf( CurrentClient, "%d\n", foundArray->NumBlocks );
			else if ( strcmp( Attribute, "ArraySizeBlocks" ) == 0 )
				fprintf( CurrentClient, "%d\n", foundArray->ArraySizeBlocks );
			else if ( strcmp( Attribute, "ArraySizeMiB" ) == 0 )
				fprintf( CurrentClient, "%f\n", foundArray->ArraySizeMiB );
			else if ( strcmp( Attribute, "ArraySizeMB" ) == 0 )
				fprintf( CurrentClient, "%f\n", foundArray->ArraySizeMB );
			else if ( strcmp( Attribute, "UsedDeviceSizeBlocks" ) == 0 )
				fprintf( CurrentClient, "%d\n", foundArray->UsedDeviceSizeBlocks );
			else if ( strcmp( Attribute, "UsedDeviceSizeMiB" ) == 0 )
				fprintf( CurrentClient, "%f\n", foundArray->UsedDeviceSizeMiB );
			else if ( strcmp( Attribute, "UsedDeviceSizeMB" ) == 0 )
				fprintf( CurrentClient, "%f\n", foundArray->UsedDeviceSizeMB );
			else if ( strcmp( Attribute, "NumRaidDevices" ) == 0 )
				fprintf( CurrentClient, "%d\n", foundArray->NumRaidDevices );
			else if ( strcmp( Attribute, "TotalDevices" ) == 0 )
				fprintf( CurrentClient, "%d\n", foundArray->TotalDevices );
			else if ( strcmp( Attribute, "PreferredMinor" ) == 0 )
				fprintf( CurrentClient, "%d\n", foundArray->PreferredMinor );
			else if ( strcmp( Attribute, "ActiveDevices" ) == 0 )
				fprintf( CurrentClient, "%d\n", foundArray->ActiveDevices );
			else if ( strcmp( Attribute, "WorkingDevices" ) == 0 )
				fprintf( CurrentClient, "%d\n", foundArray->WorkingDevices );
			else if ( strcmp( Attribute, "FailedDevices" ) == 0 )
				fprintf( CurrentClient, "%d\n", foundArray->FailedDevices );
			else if ( strcmp( Attribute, "SpareDevices" ) == 0 )
				fprintf( CurrentClient, "%d\n", foundArray->SpareDevices );
		}
	}


}

void printArrayAttributeInfo( const char* cmd ) {
	char* Attribute;

	if ( sscanf(cmd, "SoftRaid/%*[^/]/%sa", Attribute) == 1 ) {
		if ( strcmp( Attribute, "NumBlocks" ) == 0 )
			fprintf( CurrentClient, "Num blocks\t0\t10\t%%\n" );
		else if ( strcmp( Attribute, "ArraySizeBlocks" ) == 0 )
			fprintf( CurrentClient, "Used Device Size in Blocks\t0\t10\t%%\n" );
		else if ( strcmp( Attribute, "ArraySizeMiB" ) == 0 )
			fprintf( CurrentClient, "Used Device Size in MiB\t0\t10\t%%\n" );
		else if ( strcmp( Attribute, "ArraySizeMB" ) == 0 )
			fprintf( CurrentClient, "Used Device Size in MB\t0\t10\t%%\n" );
		else if ( strcmp( Attribute, "UsedDeviceSizeBlocks" ) == 0 )
			fprintf( CurrentClient, "Used Device Size in Blocks\t0\t10\t%%\n" );
		else if ( strcmp( Attribute, "UsedDeviceSizeMiB" ) == 0 )
			fprintf( CurrentClient, "Used Device Size in MiB\t0\t10\t%%\n" );
		else if ( strcmp( Attribute, "UsedDeviceSizeMB" ) == 0 )
			fprintf( CurrentClient, "Used Device Size in MB\t0\t10\t%%\n" );
		else if ( strcmp( Attribute, "NumRaidDevices" ) == 0 )
			fprintf( CurrentClient, "Total number of raid devices\t0\t10\t%%\n" );
		else if ( strcmp( Attribute, "TotalDevices" ) == 0 )
			fprintf( CurrentClient, "Total number of devices\t0\t10\t%%\n" );
		else if ( strcmp( Attribute, "PreferredMinor" ) == 0 )
			fprintf( CurrentClient, "The preferred minor\t0\t10\t%%\n" );
		else if ( strcmp( Attribute, "ActiveDevices" ) == 0 )
			fprintf( CurrentClient, "Number of active devices\t0\t10\t%%\n" );
		else if ( strcmp( Attribute, "WorkingDevices" ) == 0 )
			fprintf( CurrentClient, "Number of working devices\t0\t10\t%%\n" );
		else if ( strcmp( Attribute, "FailedDevices" ) == 0 )
			fprintf( CurrentClient, "Number of failed devices\t0\t10\t%%\n" );
		else if ( strcmp( Attribute, "SpareDevices" ) == 0 )
			fprintf( CurrentClient, "Number of spare devices\t0\t10\t%%\n" );
	}

	free(Attribute);
}



void getMdadmDetail( ArrayInfo* MyArray ) {
	int fd[2];
	pid_t ChildPID;
	int nbytes;
	
	char sensorName[128];
	char arrayDevice[ARRAYNAMELEN + 5];
	char format[ 32 ];
	char lineBuf[ 1024 ];
	char mdadmStatBuf[ MDADMSTATBUFSIZE ];	/* Buffer for mdadm --detail */
	char* mdadmStatBufP;

	/* Create a pipe */
	pipe(fd);

	/* Fork */
	if((ChildPID = fork()) == -1)
	{
		perror("Couldn't fork to launch mdadm.");
		exit(1);
	}

	/* Child will execute the program, parent will listen. */

	if (ChildPID == 0) {
	/* Child process */

		/* Child will execute the program, parent will listen. */
		/* Close stdout, duplicate the input side of pipe to stdout */
		dup2(fd[1], 1);
		/* Close output side of pipe */
		close(fd[0]);
		close(2);

		snprintf( arrayDevice, sizeof( arrayDevice ), "/dev/%s", MyArray->ArrayName );
		execl ("/sbin/mdadm", "mdadm", "--detail", arrayDevice, (char *)0);
		exit(0); /* In case /sbin/mdadm isn't found */
		/* Child is now dead, as per our request */
	}
	
	/* Parent process */
	
	/* Close input side of pipe */
	close(fd[1]);

	waitpid( ChildPID, 0, 0);
	
	/* Fill mdadmStatBuf with pipe's output */
	nbytes = read( fd[0], mdadmStatBuf, MDADMSTATBUFSIZE-1 );
	mdadmStatBuf[nbytes] = '\0';

	/* Now, go through mdadmStatBuf line by line. Register monitors along the way */
	sprintf( format, "%%%d[^\n]\n", (int)sizeof( lineBuf ) - 1 );
	mdadmStatBufP = mdadmStatBuf;
	while (sscanf(mdadmStatBufP, format, lineBuf) != EOF) {
		lineBuf[sizeof(lineBuf) - 1] = '\0';
		mdadmStatBufP += strlen(lineBuf) + 1;  /* move mdadmStatBufP to next line */
		
		if ( sscanf(lineBuf, "  Array Size : %d (%f MiB %f MB)", &MyArray->ArraySizeBlocks, &MyArray->ArraySizeMiB, &MyArray->ArraySizeMB) == 3 ) {
			MyArray->ArraySizeIsAlive = true;
			if ( !MyArray->ArraySizeIsRegistered ) {
				sprintf(sensorName, "SoftRaid/%s/ArraySizeBlocks", MyArray->ArrayName);
				registerMonitor(sensorName, "integer", printArrayAttribute, printArrayAttributeInfo, StatSM );
				sprintf(sensorName, "SoftRaid/%s/ArraySizeMiB", MyArray->ArrayName);
				registerMonitor(sensorName, "float", printArrayAttribute, printArrayAttributeInfo, StatSM );
				sprintf(sensorName, "SoftRaid/%s/ArraySizeMB", MyArray->ArrayName);
				registerMonitor(sensorName, "float", printArrayAttribute, printArrayAttributeInfo, StatSM );

				MyArray->ArraySizeIsRegistered = true;
			}
		}

		/* Versions of mdadm prior to 2.6 used "Device Size" instead of "Used Dev Size"
		 * Also, note how the if statement takes advantage of short-circuit logic.
		 */
		else if ( ( sscanf(lineBuf, " Device Size : %d (%f MiB %f MB)", &MyArray->UsedDeviceSizeBlocks, &MyArray->UsedDeviceSizeMiB, &MyArray->UsedDeviceSizeMB) == 3 ) ||
			( sscanf(lineBuf, " Used Dev Size : %d (%f MiB %f MB)", &MyArray->UsedDeviceSizeBlocks, &MyArray->UsedDeviceSizeMiB, &MyArray->UsedDeviceSizeMB) == 3 ) ) {
			MyArray->UsedDeviceSizeIsAlive = true;
			if ( !MyArray->UsedDeviceSizeIsRegistered ) {
				sprintf(sensorName, "SoftRaid/%s/UsedDeviceSizeBlocks", MyArray->ArrayName);
				registerMonitor(sensorName, "integer", printArrayAttribute, printArrayAttributeInfo, StatSM );
				sprintf(sensorName, "SoftRaid/%s/UsedDeviceSizeMiB", MyArray->ArrayName);
				registerMonitor(sensorName, "float", printArrayAttribute, printArrayAttributeInfo, StatSM );
				sprintf(sensorName, "SoftRaid/%s/UsedDeviceSizeMB", MyArray->ArrayName);
				registerMonitor(sensorName, "float", printArrayAttribute, printArrayAttributeInfo, StatSM );

				MyArray->UsedDeviceSizeIsRegistered = true;
			}
		}

		else if ( sscanf(lineBuf, "   Raid Devices : %d", &MyArray->NumRaidDevices) == 1 ) {
			MyArray->NumRaidDevicesIsAlive = true;
			if ( !MyArray->NumRaidDevicesIsRegistered ) {
				sprintf(sensorName, "SoftRaid/%s/NumRaidDevices", MyArray->ArrayName);
				registerMonitor(sensorName, "integer", printArrayAttribute, printArrayAttributeInfo, StatSM );
				MyArray->NumRaidDevicesIsRegistered = true;
			}
		}

		else if ( sscanf(lineBuf, "  Total Devices : %d", &MyArray->TotalDevices) == 1 ) {
			MyArray->TotalDevicesIsAlive = true;
			if ( !MyArray->TotalDevicesIsRegistered ) {
				sprintf(sensorName, "SoftRaid/%s/TotalDevices", MyArray->ArrayName);
				registerMonitor(sensorName, "integer", printArrayAttribute, printArrayAttributeInfo, StatSM );
				MyArray->TotalDevicesIsRegistered = true;
			}
		}

		else if ( sscanf(lineBuf, "Preferred Minor : %d", &MyArray->PreferredMinor) == 1 ) {
			MyArray->PreferredMinorIsAlive = true;
			if ( !MyArray->PreferredMinorIsRegistered ) {
				sprintf(sensorName, "SoftRaid/%s/PreferredMinor", MyArray->ArrayName);
				registerMonitor(sensorName, "integer", printArrayAttribute, printArrayAttributeInfo, StatSM );
				MyArray->PreferredMinorIsRegistered = true;
			}
		}

		else if ( sscanf(lineBuf, " Active Devices : %d", &MyArray->ActiveDevices) == 1 ) {
			MyArray->ActiveDevicesIsAlive = true;
			if ( !MyArray->ActiveDevicesIsRegistered ) {
				sprintf(sensorName, "SoftRaid/%s/ActiveDevices", MyArray->ArrayName);
				registerMonitor(sensorName, "integer", printArrayAttribute, printArrayAttributeInfo, StatSM );
				MyArray->ActiveDevicesIsRegistered = true;
			}
		}

		else if ( sscanf(lineBuf, "Working Devices : %d", &MyArray->WorkingDevices) == 1 ) {
			MyArray->WorkingDevicesIsAlive = true;
			if ( !MyArray->WorkingDevicesIsRegistered ) {
				sprintf(sensorName, "SoftRaid/%s/WorkingDevices", MyArray->ArrayName);
				registerMonitor(sensorName, "integer", printArrayAttribute, printArrayAttributeInfo, StatSM );
				MyArray->WorkingDevicesIsRegistered = true;
			}
		}

		else if ( sscanf(lineBuf, " Failed Devices : %d", &MyArray->FailedDevices) == 1 ) {
			MyArray->FailedDevicesIsAlive = true;
			if ( !MyArray->FailedDevicesIsRegistered ) {
				sprintf(sensorName, "SoftRaid/%s/FailedDevices", MyArray->ArrayName);
				registerMonitor(sensorName, "integer", printArrayAttribute, printArrayAttributeInfo, StatSM );
				MyArray->FailedDevicesIsRegistered = true;
			}
		}

		else if ( sscanf(lineBuf, " Spare Devices : %d", &MyArray->SpareDevices) == 1 ) {
			MyArray->SpareDevicesIsAlive = true;
			if ( !MyArray->SpareDevicesIsRegistered ) {
				sprintf(sensorName, "SoftRaid/%s/SpareDevices", MyArray->ArrayName);
				registerMonitor(sensorName, "integer", printArrayAttribute, printArrayAttributeInfo, StatSM );
				MyArray->SpareDevicesIsRegistered = true;
			}
		}
	}

	/* Note: Don't test NumBlocksIsAlive, because it hasn't been set yet */
	if (    (!MyArray->ArraySizeIsAlive      && MyArray->ArraySizeIsRegistered      ) ||
		(!MyArray->UsedDeviceSizeIsAlive && MyArray->UsedDeviceSizeIsRegistered ) ||
		(!MyArray->NumRaidDevicesIsAlive && MyArray->NumRaidDevicesIsRegistered ) ||
		(!MyArray->TotalDevicesIsAlive   && MyArray->TotalDevicesIsRegistered   ) ||
		(!MyArray->PreferredMinorIsAlive && MyArray->PreferredMinorIsRegistered ) ||
		(!MyArray->ActiveDevicesIsAlive  && MyArray->ActiveDevicesIsRegistered  ) ||
		(!MyArray->WorkingDevicesIsAlive && MyArray->WorkingDevicesIsRegistered ) ||
		(!MyArray->FailedDevicesIsAlive  && MyArray->FailedDevicesIsRegistered  ) ||
		(!MyArray->SpareDevicesIsAlive   && MyArray->SpareDevicesIsRegistered   )
		) {

		print_error( "RECONFIGURE" );
		
		log_error( "Soft raid device disappeared" );
		return;
	}
}

void openMdstatFile() {
	size_t n;
	int fd;

	mdstatBuf[ 0 ] = '\0';
	if ( ( fd = open( "/proc/mdstat", O_RDONLY ) ) < 0 )
		return; /* couldn't open /proc/mdstat */
	
	n = read( fd, mdstatBuf, MDSTATBUFSIZE - 1 );
	close( fd );

	if ( n == MDSTATBUFSIZE - 1 || n <= 0 ) {
		log_error( "Internal buffer too small to read \'/proc/mdstat\'" );

		return;
	}
	
	mdstatBuf[ n ] = '\0';
}

void scanForArrays() {
	char format[ 32 ];
	char buf[ 1024 ];
	char* mdstatBufP;
	char sensorName[128];

	char colonstr[128];
	
	INDEX idx;
	ArrayInfo key;

	ArrayInfo* MyArray;

	/* Mark all data as dead. As we find data, we'll mark it alive. */
	for ( MyArray = first_ctnr( ArrayInfos ); MyArray; MyArray = next_ctnr( ArrayInfos ) ) {
		MyArray->Alive = false;
	}
	MyArray = NULL;

	openMdstatFile();

	mdstatBufP = mdstatBuf;
	sprintf( format, "%%%d[^\n]\n", (int)sizeof( buf ) - 1 );

	/* Process values from /proc/mdstat */
	while (sscanf(mdstatBufP, format, buf) == 1) {
		buf[sizeof(buf) - 1] = '\0';
		mdstatBufP += strlen(buf) + 1;  /* move mdstatBufP to next line */
		
		if ( sscanf(buf, "%s %s %*s", key.ArrayName, colonstr) == 2 ) {
			if ( strcmp(colonstr, ":") == 0 && strcmp(key.ArrayName, "Personalities") != 0 ) {
				if ( ( idx = search_ctnr( ArrayInfos, ArrayInfoEqual, &key ) ) == 0 ) {
					/* Found an existing array device */
					MyArray = get_ctnr( ArrayInfos, idx );
				}
				else {
					/* Found a new array device. Create a data structure for it. */
					MyArray = calloc(1,sizeof (ArrayInfo));
					if (MyArray == NULL) {
						/* Memory could not be allocated, so print an error and exit. */
						fprintf(stderr, "Couldn't allocate memory\n");
						exit(EXIT_FAILURE);
					}

					strcpy( MyArray->ArrayName, key.ArrayName );

					/* Add this array to our list of array devices */
					push_ctnr(ArrayInfos, MyArray);
				}
				KillArrayInfo( MyArray );
				MyArray->Alive = true;

				getMdadmDetail ( MyArray );
			}
			else if (MyArray && !MyArray->NumBlocksIsAlive && (sscanf(buf, " %d blocks ", &MyArray->NumBlocks) == 1) ) {
				/* This line tells us the # of blocks. Useful b/c we didn't need mdadm --detail to get it */
				MyArray->NumBlocksIsAlive = true;
				if ( !MyArray->NumBlocksIsRegistered ) {
					sprintf(sensorName, "SoftRaid/%s/NumBlocks", MyArray->ArrayName);
					registerMonitor(sensorName, "integer", printArrayAttribute, printArrayAttributeInfo, StatSM );
					MyArray->NumBlocksIsRegistered = true;
				}
			}
		}
	}
	
	/* Look for dead arrays, and for NumBlocksIsRegistered */
	for ( MyArray = first_ctnr( ArrayInfos ); MyArray; MyArray = next_ctnr( ArrayInfos ) ) {
		if ( ( MyArray->Alive = false ) || ( !MyArray->NumBlocksIsAlive && MyArray->NumBlocksIsRegistered ) ) {
			print_error( "RECONFIGURE" );
			
			log_error( "Soft raid device disappeared" );
			return;
		}
	}
}

/* =========== Public part =========== */

void initSoftRaid( struct SensorModul* sm ) {
  	StatSM = sm;

	ArrayInfos = new_ctnr();

	updateSoftRaid();
}

void exitSoftRaid( void ) {
	destr_ctnr( ArrayInfos, free );
}

int updateSoftRaid( void ) {
	scanForArrays();
	return 0;
}