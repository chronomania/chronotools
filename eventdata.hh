#include <utility>
#include <string>
#include <vector>

extern const char* LocationEventNames[0x201];
extern const char* MapNames[0x200];
extern const char* Emotion[0x1B];
extern const char* ActorNames[0x100];
extern const char* SoundEffectNames[0x100];
extern const char* SongNames[0x53];


class EventCode
{
public:
    EventCode();
    ~EventCode();
    
    void InitDecode(unsigned offset, unsigned char opcode);
    
    struct DecodeResult
    {
        std::string code;
        unsigned nbytes;
        
        /* If the code contains a label, these fields
         * tell what the label was called (for str_replace)
         * and what location it points to.
         */
        std::string label_name;
        unsigned    label_value;
    };
    
    DecodeResult DecodeBytes(const unsigned char* data, unsigned maxlength);
    
    struct EncodeResult
    {
        std::vector<unsigned char> result;
        
        /* If the code contained a label, these fields
         * tell what the label was called (for table lookup),
         * what byte position it's compiled to
         * and whether the value should be incremented or decremented.
         */
        std::string label_name;
        unsigned    label_position;
        bool        label_forward;
    };
    EncodeResult EncodeCommand(const std::string& cmd);

private:
    EventCode(const EventCode& );
    void operator=(const EventCode& );

private:
    class EvParameterHandler* ev;
};
