#include <stdlib.h>
#include <stdio.h>

#include <bgfx_utils.h>
#include <common.h>

#include "gfx.h"

int _main_(int argc, char **argv) {
    return Gfx::create()->run(argc, argv);
}
