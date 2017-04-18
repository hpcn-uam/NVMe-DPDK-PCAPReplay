#ifndef __common_h__
#define __common_h__

#define UNUSED(X) ((void)(X))

#include <fs.h>
#include <simpleio.h>
#include <spcap.h>

// Spdk
#include "spdk/env.h"

static const char pcapExt[] = ".nscap";

void app_config (int argc, char **argv, struct spdk_env_opts *conf);
void app_init (nvmeRaid *raid);
void app_run (nvmeRaid *raid);

#endif