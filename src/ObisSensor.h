#include <list>
#include "SoftwareSerial.h"
#include "ObisHelp.h"
#include "Prometheus.h"

using namespace std;

// OBIS message patterns
const byte START_SEQUENCE[] = {0x2F};
const byte END_SEQUENCE[] = {0x21, 0x0D, 0x0A};
const size_t BUFFER_SIZE = 3840;


bool isNumeric(String text)
{
    for (size_t i = 0; i < text.length(); i++)
    {
        if (!isdigit(text[i]) && text[i] != '.' && text[i] != '-')
        {
            return false;
        }
    }    
    return true;
}

bool isObis(String text)
{
    for (size_t i = 0; i < text.length(); i++)
    {
        // No leading zeroes
        if (i > 0 && isdigit(text[i]) && text[i-1] == '0')
        {
            return false;
        }

        // Only numbers and dots
        if (!isdigit(text[i]) && text[i] != '.')
        {
            return false;
        }
    }    
    return true;
}


bool extract_gauges(std::list<Gauge> *gauges, size_t last_message_size, char *buffer)
{
    String device_id = "";
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
            if (!isObis(obis_code))
            {
                // Skip this row because of transmission errors in the OBIS code
                phase = 0;
            }
            else
            {
                phase = 2;
            }
        }
        else if (phase == 2 && buffer[i] == '(')
        {
            // Value coming up
            phase = 3;
        }
        else if (phase == 3 && (buffer[i] == '*' || buffer[i] == ')'))
        {
            if (obis_code == "0.0.0")
            {
                device_id = value;
            }

            // Only proceed if we have the serial number.
            // Otherweise the metric attributes would be incomplete!
            if (device_id.length() == 0)
            {
                // Abort extraction because the device ID was no identified
                Serial.println("ERR: Device ID not found.");
                gauges->clear();
                return false;
            }

            // Summarize this as a Prometheus Gauge object
            if (isNumeric(value))
            {
                // Prometheus Gauges must be numeric
                metric_name = "obis_" + obis_code;
                metric_name.replace('.', '_');
                metric_help = get_obis_help(obis_code);
                Gauge g = Gauge(metric_name, metric_help, "serial=\"" + device_id + "\"");
                g.set(value);

                // Collect
                gauges->push_back(g);
            }
            else
            {
                Serial.println("Skipped " + obis_code + " with non-numeric value '" + value + "'.");
            }

            // back to "search" phase
            phase = 0;
        }
        else if (buffer[i] == '\n' || buffer[i] == '\r')
        {
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
    return true;
}

class ObisSensor
{
    public:
    char *serialnumber;
    std::list<Gauge> *gauges = new std::list<Gauge>();
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

    void interrupt()
    {
        // This is called when the webserver processes a request,
        // to notify the data receival loop to discard any ongoing
        // message transfer, which could be corrupted by the distraction.
        found_start = false;
    }

    private:
    unique_ptr<SoftwareSerial> serial;
    byte buffer[BUFFER_SIZE];
    bool found_start = false;
    size_t position = 0;
    size_t last_message_size = 0;

    void read()
    {
        while (serial->available())
        {
            buffer[position] = serial->read();

            // Is this the start marker "/"?
            if (buffer[position] == '/')
            {
                // Start marker encountered!
                position = 0;
                found_start = true;
            }
            else if (found_start && buffer[position] == '!')
            {
                // End marker encountered!
                last_message_size = position;
                process_message();
                position = 0;
                found_start = false;
            }
            else if (position == BUFFER_SIZE - 2)
            {
                position = 0;
                found_start = false;
            }
            position += 1;

        }
        yield();
    }

    void process_message()
    {
        if (last_message_size == 0)
        {
            return;
        }

        gauges->clear();
        bool success = extract_gauges(gauges, last_message_size, (char*) buffer);
        if (success)
        {
            metrics = render(gauges);
            Serial.println("Rendered " + String(gauges->size()) + " metrics.");
        }
    }
};
