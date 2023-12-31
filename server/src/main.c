#include <stdio.h>
#include <stdlib.h>

#include "logging.h"

int main(int argc, char *argv[])
{
        int rv = 1;

        if_failed(init_logging(), exit);

        uninit_logging();

        rv = 0;
exit:
        return rv;
}
