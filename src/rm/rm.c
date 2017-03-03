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
	    "rm");
}

size_t n_files;
char const *const *file;

static void app_paramCheck (void) {
	int stopExecution = 0;

	if (n_files == 0) {
		stopExecution = 1;
		printf ("PARAM-ERROR: Need to pass at least one file to remove\n");
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
	// clean a bit the screen
	puts ("");
	puts ("");
	return;
}
void app_run (nvmeRaid *raid) {
	size_t i;

	for (i = 0; i < n_files; i++) {
		printf ("Trying to remove file \"%26s\"...", file[i]);
		if (delFile (raid, file[i])) {
			printf ("OK\n");
		} else {
			printf ("ERROR\n");
		}
	}
	return;
}
