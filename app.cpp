#include <stdio.h>
#include "system.h"
#include "pico/stdlib.h"
#include "taskhandler.h"

int main() {
    initSystem();
    createTasks();
    startTasks();
}
