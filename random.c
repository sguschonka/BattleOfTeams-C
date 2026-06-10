#include "random.h"
#include <stdlib.h>
#include <time.h>

void init_random() {
    srand(time(NULL));
}

int random_range(int min, int max) {
    return min + rand() % (max - min + 1);
}