
String get_obis_help(String obis_code)
{
    if (obis_code == "0.0.0") {
        return "Device identifier";
    }
    if (obis_code == "96.1.0") {
        return "Device identification";
    }
    if (obis_code == "1.8.0") {
        return "Consumed energy";
    }
    if (obis_code == "2.8.0") {
        return "Provided energy";
    }
    if (obis_code == "16.7.0") {
        return "Current power";
    }
    if (obis_code == "36.7.0") {
        return "Current power on L1";
    }
    if (obis_code == "56.7.0") {
        return "Current power on L2";
    }
    if (obis_code == "76.7.0") {
        return "Current power on L3";
    }
    if (obis_code == "32.7.0") {
        return "Voltage on L1";
    }
    if (obis_code == "52.7.0") {
        return "Voltage on L2";
    }
    if (obis_code == "72.7.0") {
        return "Voltage on L3";
    }
    if (obis_code == "96.5.0") {
        return "Device status";
    }
    if (obis_code == "96.8.0") {
        return "Device time of operation";
    }
    return "";
}
