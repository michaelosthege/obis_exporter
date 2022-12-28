class Metric
{
    public:
    String Name;
    String Help;

    virtual String toString() = 0;
};


class Gauge : public virtual Metric
{
    public:
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

String render(std::list<Gauge> *gauges)
{
    String metrics = "";
    for (std::list<Gauge>::iterator g = gauges->begin(); g != gauges->end(); ++g)
    {
        metrics += g->toString();
    }
    return metrics;
}
