#include <Arduino.h>
#include "core/AshuraCore.h"
#include "storage/AshuraPrefs.h"

AshuraCore core;

void setup(){
    core.init();
}

void loop(){
    core.update();
}