#include <ctime>

using namespace std;

#include "msginsert.hh"
#include "ctinsert.hh"

namespace
{
    unsigned prev_sect_len = 0;
    bool     working       = false;
    
    void Working()
    {
        static const char animation[4] = {'-', '\\', '|', '/' };
        
        static unsigned shift   = 1;
        static unsigned curspos = 0;
        static int shift_estimatecount = 0, shift_threshold = 10;
        static time_t estimate_begintime;
        
        if(!working)
        {
            // If we just restarted, ensure it won't take too long
            if(shift_estimatecount + 10 < shift_threshold)
            {
                // Reset to default
                shift_estimatecount = 0;
                shift_threshold     = 10;
                // Don't touch "shift".
            }
            // Ensure we get at least one output
            curspos = (curspos / shift) * shift;
        }
        working = true;
        
        if(shift_estimatecount >= 0)
        {
            if(shift_estimatecount == 0)
            {
                estimate_begintime = time(NULL);
                shift_threshold    = 10; // initial threshold.
            }
            
            ++shift_estimatecount;
            if(shift_estimatecount > shift_threshold)
            {
                double elapsed = difftime(time(NULL), estimate_begintime);
                if(elapsed < 1)
                {
                    /* Too short time, must gather more samples */
                    shift_threshold *= 2;
                    unsigned shift_estimate = shift_threshold / 5;
                    if(shift < shift_estimate)
                        shift = shift_estimate;
                }
                else
                {
                    /* Shift so that it does about one cycle per second */
                    shift = (unsigned) (shift_estimatecount / (elapsed*4));
                    /* Don't divide by zero */
                    if(shift <= 0) shift = 1;
                    
                    /* Start new round */
                    shift_estimatecount = 0;
                }
            }
        }
        
        if(!(curspos%shift)) fprintf(stderr, "%c\010", animation[curspos / shift]);
        curspos = (curspos+1) % (4*shift);
    }
    
    void Notworking()
    {
        if(working)
        {
            fprintf(stderr, " \010");
            working = false;
        }
    }
    
    void Error()
    {
        Notworking();
        fprintf(stderr, " -\n");
    }
    
    void Resume()
    {
        prev_sect_len = 0;
        working       = false;
    }
    
    void Back()
    {
        Notworking();
        string s(prev_sect_len, '\010');
        fprintf(stderr, "%-*s%s", s.size()*2, s.c_str(), s.c_str());
        Resume();
    }
    
    void SectionMessage(const string& header)
    {
        Back();
        fprintf(stderr, "%s", header.c_str());
        prev_sect_len = header.size();
    }
}

void MessageLoadingDialog()
{
    fprintf(stderr, "Loading dialog... ");
    Resume();
}
void MessageLoadingImages()
{
    fprintf(stderr, "Loading images... ");
    Resume();
}
void MessageApplyingDictionary()
{
    fprintf(stderr, "Applying dictionary... ");
    Resume();
}
void MessageMeasuringScript()
{
    fprintf(stderr, "Calculating script size... ");
    Resume();
}
void MessageReorganizingFonts()
{
    fprintf(stderr, "Analyzing character usage... ");
    Resume();
}
void MessageLinkingModules(unsigned count)
{
    fprintf(stderr, "Linking %u modules... ", count);
    Resume();
}

void MessageLoadingItem(const string& header)
{
    SectionMessage(header);
}
void MessageZSection(const string& header)
{
    SectionMessage(header);
}
void MessageRSection(const string& header)
{
    SectionMessage(header);
}
void MessageLSection(const string& header)
{
    SectionMessage(header);
}
void MessageDSection(const string& header)
{
    SectionMessage(header);
}
void MessageSSection(const string& header)
{
    SectionMessage(header);
}
void MessageUnknownHeader(const string& header)
{
    Error();
    fprintf(stderr, "Unknown header '%s'...", header.c_str());
    Resume();
}
void MessageInvalidLabelChar(ucs4 c, unsigned label, const string& header)
{
    Error();
    fprintf(stderr,
            "$%X: Got char '%c', invalid is (in label)!", label, c);
    Resume();
}
void MessageInvalidLabelChar(ucs4 c, const string& label, const string& header)
{
    Error();
    fprintf(stderr,
           "$%s: Got char '%c', expected ':' (in label)!", label.c_str(), c);
    Resume();
}
void MessageWorking()
{
    Working();
}
void MessageUnexpected(const ucs4string& unexpected)
{
    Error();
    fprintf(stderr, "Unexpected data '%s'", WstrToAsc(unexpected).c_str());
    Resume();
}
void MessageDone()
{
    Back();
    fprintf(stderr, "done.\n");
}
void MessageSymbolIgnored(const ucs4string& symbol)
{
    Error();
    fprintf(stderr,
        "Warning: Symbol '%s' wouldn't work in different typefaces!\n"
        "         Thus not generating.",
            WstrToAsc(symbol).c_str());
    Resume();
}
void MessageTFStartError(const ucs4string& tf)
{
    Error();
    fprintf(stderr, "Error: Started typeface with '%s' when one was already active.",
            WstrToAsc(tf).c_str());
    Resume();
}
void MessageTFEndError(const ucs4string& tf)
{
    Error();
    fprintf(stderr, "Error: Ended typeface with '%s' when none was active.",
            WstrToAsc(tf).c_str());
    Resume();
}
void MessageTooLongText(const ctstring& input, const ctstring& output)
{
    Error();
    fprintf(stderr,
        "Error: Too long text\n"
        "In:  %s\n"
        "Out: %s\n"
        "... ",
        DispString(input).c_str(),
        DispString(output).c_str());
    Resume();
}
