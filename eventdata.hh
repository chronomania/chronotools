#include <utility>
#include <string>
#include <vector>

#include "wstring.hh"

extern const char* LocationEventNames[0x201];
extern const char* MapNames[0x200];
extern const char* Emotion[0x20];
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
        
        /* If the code contained a goto of some kind, this field
         * tells which byte position should the goto be poked in.
         * The caller should know whether there is a goto or not.
         */
        unsigned goto_position;
    };
    
    const DecodeResult
    DecodeBytes(unsigned offset, const unsigned char* data, unsigned maxlength);
    
    const EncodeResult
    EncodeCommand(const std::wstring& cmd, bool goto_backward=false);

public:
    struct DecodingState
    {
        unsigned dialogbegin;
    } DecodeState;

    struct EncodingState
    {
        unsigned dialogbegin;
    } EncodeState;
private:
    EventCode(const EventCode& );
    void operator=(const EventCode& );
};
