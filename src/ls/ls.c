#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rte_config.h>
#include <rte_eal.h>

#include "spdk/nvme.h"
#include "spdk/env.h"

#include <common.h>

void app_config (int argc, char **argv) {
	UNUSED (argc);
	UNUSED (argv);
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
	}
	return;
}
