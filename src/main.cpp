#include <Arduino.h>
#include "core/AshuraCore.h"

AshuraCore core;

void setup(){
    core.init();
}

void loop(){
    core.update();
}