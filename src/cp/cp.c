#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include <rte_config.h>
#include <rte_eal.h>

#include "spdk/nvme.h"
#include "spdk/env.h"

#include <common.h>

static void app_usage (void) {
	printf (
	    "This is a NVME-DPDK-PCAPRelplay %s tool\n"
	    "\n"
	    "Available options are:"
	    "--help : To show this help info\n"
	    "--nscap : Interpretate the file as a NVME-SPDK-PCAP / PCAP.\n"
	    "--from-sys  [filename]: Specifies the origin file from the system\n"
	    "--from-raid [filename]: Specifies the origin file from the NVME-RAID-FS\n"
	    "--to-sys    [filename]: Specifies the destination file to the system\n"
	    "--to-raid   [filename]: Specifies the destination file to the NVME-RAID-FS\n",
	    "cp");
}

int ffrom_sys = 0, ffrom_raid = 0, fto_sys = 0, fto_raid = 0, fpcap = 0;
char *cfrom_sys = NULL, *cfrom_raid = NULL, *cto_sys = NULL, *cto_raid = NULL;

static void app_paramCheck (void) {
	int stopExecution = 0;
	if (ffrom_sys && fto_sys) {
		printf ("PARAM-ERROR: Internal system cp not yet implemented\n");
		stopExecution = 1;
	}
	if (ffrom_raid && fto_raid) {
		printf ("PARAM-ERROR: Internal raid cp not yet implemented\n");
		stopExecution = 1;
	}
	if (ffrom_sys && !fto_raid) {
		printf ("PARAM-ERROR: Need to provide a target raid filename\n");
		stopExecution = 1;
	}
	if (ffrom_raid && !fto_sys) {
		printf ("PARAM-ERROR: Need to provide a target system filename\n");
		stopExecution = 1;
	}
	if (!ffrom_sys && fto_raid) {
		printf ("PARAM-ERROR: Need to provide a origin system filename\n");
		stopExecution = 1;
	}
	if (!ffrom_raid && fto_sys) {
		printf ("PARAM-ERROR: Need to provide a origin raid filename\n");
		stopExecution = 1;
	}
	if (!ffrom_sys && !ffrom_raid && !fto_sys && !fto_raid) {
		printf ("PARAM-ERROR: No param provided\n");
		stopExecution = 1;
	}
	if (ffrom_sys && ffrom_raid) {
		printf ("PARAM-ERROR: 2 froms provided\n");
		stopExecution = 1;
	}
	if (fto_sys && fto_raid) {
		printf ("PARAM-ERROR: 2 destinations provided\n");
		stopExecution = 1;
	}

	if (stopExecution) {
		printf ("\n");
		app_usage ();
		exit (-1);
	}
}

void app_config (int argc, char **argv) {
	int c;
	while (1) {
		static struct option long_options[] = {{"help", no_argument, 0, 'h'},
		                                       {"pcap", no_argument, 0, 'p'},
		                                       {"from-sys", required_argument, 0, 'y'},
		                                       {"from-raid", required_argument, 0, 'f'},
		                                       {"to-sys", required_argument, 0, 's'},
		                                       {"to-raid", required_argument, 0, 't'},
		                                       {0, 0, 0, 0}};
		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long (argc, argv, "hps:t:y:f:", long_options, &option_index);

		/* Detect the end of the options. */
		if (c == -1)
			break;

		switch (c) {
			case 0:
				/* If this option set a flag, do nothing else now. */
				if (long_options[option_index].flag != 0)
					break;
				printf ("option %s", long_options[option_index].name);
				if (optarg)
					printf (" with arg %s", optarg);
				printf ("\n");
				break;

			case 'y':  // from-sys
				ffrom_sys = 1;
				cfrom_sys = strdup (optarg);
				break;

			case 'f':  // from-raid
				ffrom_raid = 1;
				cfrom_raid = strdup (optarg);
				if (strlen (cfrom_raid) > FILENAME_MAX) {
					printf ("The raid's filename is too long, limited to %d chars\n", FILENAME_MAX);
					exit (-1);
				}
				break;

			case 's':  // to-sys
				fto_sys = 1;
				cto_sys = strdup (optarg);
				break;

			case 't':  // to-raid
				fto_raid = 1;
				cto_raid = strdup (optarg);
				if (strlen (cto_raid) > FILENAME_MAX) {
					printf ("The raid's filename is too long, limited to %d chars\n", FILENAME_MAX);
					exit (-1);
				}
				break;

			case 'p':  // to-sys
				fpcap = 1;
				break;

			case 'h':
			case '?':
			default:
				app_usage ();
				exit (1);
		}
	}
	app_paramCheck ();
	return;
}
void app_init (nvmeRaid *raid) {
	UNUSED (raid);
	// clean a bit the screen
	puts ("");
	puts ("");
	return;
}
void app_run (nvmeRaid *raid) {
	uint64_t origin_size;
	uint64_t origin_size_blks;
	metaFile *raid_file;
	if (ffrom_sys && fto_raid) {
		// check if origin file exists
		FILE *f = fopen (cfrom_sys, "r");
		if (f == NULL) {
			printf ("The file %s does not exists\n", cfrom_sys);
			return;
		}
		fseek (f, 0L, SEEK_END);
		origin_size = ftell (f);
		fseek (f, 0L, SEEK_SET);  // = rewind

		origin_size_blks = origin_size / METASECTORLENGTH;
		if (origin_size % METASECTORLENGTH != 0) {
			origin_size_blks++;
			printf ("The file %s is not alligned with sectorsize (%lu), padding may be added\n",
			        cfrom_sys,
			        METASECTORLENGTH);
		}

		// check if file exists in the raid
		raid_file = findFile (raid, cto_raid);
		if (raid_file) {  // file exists
			uint64_t fsize = raid_file->endBlock - raid_file->startBlock;
			fsize *= METASECTORLENGTH;

			// check if new file is smaller or greater than newer one
			if (origin_size > fsize) {
				printf (
				    "The file going to write (%lu B) is bigger than the one is now written "
				    "in the raid (%lu B). This is not supported\n",
				    origin_size,
				    fsize);
				return;
			}

		} else {  // file does not exists
			raid_file = addFile (raid, cto_raid, origin_size_blks);
			if (raid_file) {
				printf("Can not allocate a new file in the NVMe-raid\n");
			}
		}

	} else if (ffrom_raid && fto_sys) {
	}
	return;
}
