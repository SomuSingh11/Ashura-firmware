#include <Arduino.h>
#include "core/AppManager.h"

AppManager app;

void setup(){
    app.init();
}

void loop(){
    app.update();
}