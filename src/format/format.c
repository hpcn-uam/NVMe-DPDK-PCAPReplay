#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
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
	    "This is a NVME-DPDK-PCAPReplay %s tool\n"
	    "\n"
	    "This tool will erase all the raid contents by overwritting sector 0\n",
	    "format");
}

int ffrom_sys = 0, ffrom_raid = 0, fto_sys = 0, fto_raid = 0, fpcap = 0;
char *cfrom_sys = NULL, *cfrom_raid = NULL, *cto_sys = NULL, *cto_raid = NULL;

static void app_paramCheck (void) {
	int stopExecution = 0;

	if (stopExecution) {
		printf ("\n");
		app_usage ();
		exit (-1);
	}
}

void app_config (int argc, char **argv, struct spdk_env_opts *conf) {
	UNUSED(conf);
	
	int c;
	while (1) {
		static struct option long_options[] = {{"help", no_argument, 0, 'h'}, {0, 0, 0, 0}};
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
	// format
	formatRaid (raid);
	return;
}
void app_run (nvmeRaid *raid) {
	UNUSED (raid);
	return;
}
