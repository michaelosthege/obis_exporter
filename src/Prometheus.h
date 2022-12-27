class Gauge
{
    public:
    String Name;
    String Help;
    String Value;

    Gauge(String name, String help)
    {
        this->Name = name;
        this->Help = help;
    }

    void set(String value)
    {
        this->Value = value;
    }

    String toString()
    {
        return (
            "# " + Name + " " + Help + "\n" +
            "# TYPE " + Name + " gauge\n" +
            Name + " " + Value + "\n"
        );
    }
};

String render(std::list<Gauge *> *gauges)
{
    String metrics = "";
    for (std::list<Gauge*>::iterator g = gauges->begin(); g != gauges->end(); ++g)
    {
        metrics += (*g)->toString();
    }
    return metrics;
}
