#include <list>
#include "SoftwareSerial.h"
#include "ObisHelp.h"
#include "Prometheus.h"

using namespace std;

// OBIS message patterns
const byte START_SEQUENCE[] = {0x2F};
const byte END_SEQUENCE[] = {0x21, 0x0D, 0x0A};
const size_t BUFFER_SIZE = 3840;


void extract_gauges(std::list<Gauge *> *gauges, size_t last_message_size, char *buffer)
{
    String obis_code = "";
    String metric_name = "";
    String metric_help = "";
    String value = "";

    // Extract current metrics from message
    uint8 phase = 0;  // 0: searching, 1: obis code, 2: value
    for (size_t i = 0; i < last_message_size; i++)
    {
        // Transitions between segments of the input text
        if (phase == 0 && buffer[i] == ':')
        {
            // OBIS code coming up
            phase = 1;
            obis_code = "";
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
            // Summarize this as a Prometheus Gauge object
            metric_name = "obis_" + obis_code;
            metric_name.replace('.', '_');
            metric_help = get_obis_help(obis_code);
            Gauge *g = new Gauge(metric_name, metric_help);
            g->set(value);

            // Collect
            gauges->push_back(g);

            // back to "search" phase
            phase = 0;
        }
        else
        {
            // Not a transition character -> Collect
            if (phase == 1)
            {
                obis_code += char(buffer[i]);
            }
            else if (phase == 3)
            {
                value += char(buffer[i]);
            }
        }
    }
}

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
        extract_gauges(gauges, last_message_size, (char*) buffer);

        metrics = render(gauges);

        Serial.println(metrics);
    }
};