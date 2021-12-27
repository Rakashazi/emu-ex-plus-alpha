#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#define trio_snprintf snprintf
#define trio_fprintf fprintf
#define trio_sscanf sscanf
#define trio_fscanf fscanf
#define trio_vasprintf vasprintf
