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
	    "Available options are:\n"
	    "--help : To show this help info\n",
	    "ls");
}

size_t n_files;
char const *const *file;

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

		c = getopt_long (argc, argv, ":h", long_options, &option_index);
		/* Detect the end of the options. */
		if (c == -1)
			break;

		switch (c) {
			case 'h':
			case '?':
			default:
				puts ("default");
				app_usage ();
				exit (1);
		}
	}

	n_files = argc - optind;
	file    = (char const *const *)argv + optind;

	app_paramCheck ();
	return;
}
void app_init (nvmeRaid *raid) {
	UNUSED (raid);
	return;
}
void app_run (nvmeRaid *raid) {
	if (raid->numFiles == 0) {
		printf ("There is no files in the raid\n");
	} else {
		int i, j;
		int id = 0;
		for (i = 0; i < raid->numdisks; i++) {
			for (j = 0; j < MAXFILES; j++, id++) {
				if (raid->disk[i].msector.content[j].name[0] != 0) {
					printf ("%02d: %26s\t%lu Sectors\n",
					        id,
					        raid->disk[i].msector.content[j].name,
					        raid->disk[i].msector.content[j].endBlock -
					            raid->disk[i].msector.content[j].startBlock);
				}
			}
		}
		printf ("\n Showing all %d files.\n", raid->numFiles);
		printf ("\n %lu Block reserved/used from %lu total (%lf %%)\n",
		        raid->totalBlocks - rightFreeBlocks (raid),
		        raid->totalBlocks,
		        100*((double)raid->totalBlocks - rightFreeBlocks (raid)) / ((double)rightFreeBlocks (raid)));
	}
	return;
}
