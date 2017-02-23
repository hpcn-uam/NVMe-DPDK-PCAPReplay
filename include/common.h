#ifndef __common_h__
#define __common_h__

#define UNUSED(X) ((void) (X))

#include <fs.h>
#include <simpleio.h>

void app_config (int argc, char **argv);
void app_init (void);
void app_run (void);

#endif