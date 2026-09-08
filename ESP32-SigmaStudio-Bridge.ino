#include "Adau146xController.h"

void setup() {
    controller.begin();
}

void loop() {
    controller.run();
}
