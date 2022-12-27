#include <list>
#include "SoftwareSerial.h"
#include "Prometheus.h"

using namespace std;

// OBIS message patterns
const byte START_SEQUENCE[] = {0x2F};
const byte END_SEQUENCE[] = {0x21, 0x0D, 0x0A};
const size_t BUFFER_SIZE = 3840;

class ObisSensor
{
    public:
    char *serialnumber;
    std::list<Gauge *> *gauges = new std::list<Gauge *>();
    String metrics = "";

    ObisSensor(uint8_t rx_pin)
    {
        serial = unique_ptr<SoftwareSerial>(new SoftwareSerial());
        serial->begin(9600, SWSERIAL_7E1, rx_pin, -1, false);
        serial->enableTx(false);
        serial->enableRx(true);
    }

    void loop()
    {
        read();
    }

    private:
    unique_ptr<SoftwareSerial> serial;
    byte buffer[BUFFER_SIZE];
    size_t last_message_size = 0;
    String name = "obis_";
    String value = "";

    void read()
    {
        while (serial->available())
        {
            // Read one byte into the first position of the buffer
            buffer[0] = serial->read();

            // Is this the start marker "/"?
            if (buffer[0] == 0x21)
            {
                // Read until the end marker
                last_message_size = serial->readBytesUntil('!', (char*) buffer, BUFFER_SIZE);
                process_message();
            }

            yield();
        }
    }

    void process_message()
    {
        if (last_message_size == 0)
        {
            return;
        }

        gauges->clear();
        extract_gauges(gauges, last_message_size);
        

        metrics = render(gauges);

        Serial.println(metrics);

    void extract_gauges(std::list<Gauge *> *gauges, size_t last_message_size)
    {
        // Extract current metrics from message
        uint8 phase = 0;  // 0: searching, 1: obis code, 2: value
        for (size_t i = 0; i < last_message_size; i++)
        {
            // Transitions between segments of the input text
            if (phase == 0 && buffer[i] == ':')
            {
                // OBIS code coming up
                phase = 1;
                name = "obis_";
                value = "";
            }
            else if (phase == 1 && buffer[i] == '*')
            {
                // End of OBIS code
                phase = 2;
            }
            else if (phase == 2 && buffer[i] == '(')
            {
                // Value coming up
                phase = 3;
            }
            else if (phase == 3 && (buffer[i] == '*' || buffer[i] == ')'))
            {
                // Summarize this line in openmetrics format
                Gauge *g = new Gauge(name, "");
                g->set(value);
                gauges->push_back(g);

                // back to "search" phase
                phase = 0;
            }
            else
            {
                // Not a transition character -> Collect
                if (phase == 1)
                {
                    if (buffer[i] == '.')
                    {
                        name += '_';
                    }
                    else
                    {
                        name += char(buffer[i]);
                    }
                }
                else if (phase == 3)
                {
                    value += char(buffer[i]);
                }
            }
        }
    }
};
