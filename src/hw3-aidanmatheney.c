/*
 * Aidan Matheney
 * aidan.matheney@und.edu
 *
 * CSCI 451 HW3
 */

#include "../include/hw3.h"

#include <stdlib.h>
#include <stdio.h>

int main(int const argc, char ** const argv) {
    HW3Result const hw3Result = hw3("http://undcemcs01.und.edu/~ronald.marsh/CLASS/CS451/hw3-data.txt");
    if (!HW3Result_isSuccess(hw3Result)) {
        fprintf(
            stderr,
            "ERROR: Failed to download HW3 data (wget exit code: %d)\n",
            HW3Result_getErrorAndDestroy(hw3Result)
        );
        return EXIT_FAILURE;
    }
    HW3Result_destroy(hw3Result);

    return EXIT_SUCCESS;
}
