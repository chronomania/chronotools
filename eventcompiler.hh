#include <string>
#include <vector>

#include "wstring.hh"

class EventCompiler
{
public:
    EventCompiler();
    ~EventCompiler();

    void AddData(unsigned ptr_addr,
                 const std::string& label,
                 const std::wstring& data);
    void Close();

public:
    struct Event
    {
        unsigned ptr_addr;
        std::vector<unsigned char> data;
    };
    std::vector<Event> events;

private:
    void operator=(const EventCompiler&);
    EventCompiler(const EventCompiler&);
    
private:
    void FinishCurrent();
private:
    unsigned last_addr;
    class LocationEvent* cur_ev;
};
