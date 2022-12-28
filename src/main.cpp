#include <list>
#include "ObisSensor.h"

std::list<ObisSensor *> *sensors = new std::list<ObisSensor *>();

void setup()
{
    // Set up debugging via serial
    Serial.begin(115200);

    // Configure IR sensors
    Serial.println("Setting up sensors");
    sensors->push_back(new ObisSensor(D2));
    sensors->push_back(new ObisSensor(D5));

    Serial.println("Setup completed.");
}

void loop()
{
    // Execute sensor state machines
    for (std::list<ObisSensor*>::iterator it = sensors->begin(); it != sensors->end(); ++it)
    {
        (*it)->loop();
    }

    yield();
}
