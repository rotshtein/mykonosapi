#pragma once

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "t_mykonos.h"

char* chnage_frequenct_last_error();
mykonosErr_t change_frequency(mykonosDevice_t *mykDevice, uint64_t new_rx_freq, uint64_t new_tx_freq);

