#include <utility>
#include <string>
#include <vector>

#include "wstring.hh"

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
    
    enum gototype
    {
        goto_none,
        goto_backward,
        goto_forward,
        goto_if,
        goto_ifnot,
        goto_loopbegin /* pseudo goto */
    };

    struct DecodeResult
    {
        std::wstring code;
        std::wstring annotation;
        unsigned nbytes;
        
        /* If the code contains a label, these fields
         * tell what the label was called (for str_replace)
         * and what location it points to.
         */
        unsigned goto_target;
        gototype goto_type;
    };
    
    struct EncodeResult
    {
        std::vector<unsigned char> result;
        
        /* If the code contained a label, these fields
         * tell what the label was called (for table lookup),
         * what byte position it's compiled to
         * and whether the value should be incremented or decremented.
         */
        std::wstring label_name;
        unsigned     label_position;
        bool         label_forward;
    };
    
    const DecodeResult
    DecodeBytes(unsigned offset, const unsigned char* data, unsigned maxlength);
    
    const EncodeResult
    EncodeCommand(const std::wstring& cmd);

public:
    struct DecodingState
    {
        unsigned dialogbegin;
    } DecodeState;
private:
    EventCode(const EventCode& );
    void operator=(const EventCode& );
};
