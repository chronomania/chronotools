#include <ctime>
#include <sstream>

using namespace std;

#include "msginsert.hh"
#include "ctinsert.hh"

namespace
{
    unsigned prev_sect_len = 0;
    bool     working       = false;
    
    static const char backspace = (char)8;
    
    void RenderCursor(char c)
    {
        fprintf(stderr, "%c%c", c, backspace);
    }
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
        
        if(!(curspos%shift)) RenderCursor(animation[curspos/shift]);
        curspos = (curspos+1) % (4*shift);
    }
    
    void Notworking()
    {
        if(working)
        {
            RenderCursor(' ');
            working = false;
        }
    }
    
    void NewBeginning()
    {
        prev_sect_len = 0;
        working       = false;
    }
    
    void UndoSection()
    {
        Notworking();
        
        std::stringstream str;
        for(unsigned a=0; a<prev_sect_len; ++a) str << backspace;
        for(unsigned a=0; a<prev_sect_len; ++a) str << ' ';
        for(unsigned a=0; a<prev_sect_len; ++a) str << backspace;
        fprintf(stderr, "%s", str.str().c_str());
        fflush(stderr);
        
        NewBeginning();
    }
    
    void SectionMessage(const std::string& header)
    {
        UndoSection();
        fprintf(stderr, "%s", header.c_str());
        fflush(stderr);
        prev_sect_len = header.size();
    }

    void BeginError()
    {
        Notworking();
        fprintf(stderr, "*\n");
    }
}

void MessageLoadingDialog()
{
    fprintf(stderr, "Loading dialog... ");
    NewBeginning();
}
void MessageLoadingImages()
{
    fprintf(stderr, "Loading images... ");
    NewBeginning();
}
void MessageApplyingDictionary()
{
    fprintf(stderr, "Applying dictionary... ");
    NewBeginning();
}
void MessageMeasuringScript()
{
    fprintf(stderr, "Calculating script size... ");
    NewBeginning();
}
void MessageReorganizingFonts()
{
    fprintf(stderr, "Analyzing character usage... ");
    NewBeginning();
}
void MessageLinkingModules(unsigned count)
{
    fprintf(stderr, "Linking %u modules... ", count);
    NewBeginning();
}
void MessageWritingStrings()
{
    fprintf(stderr, "Writing strings... ");
    NewBeginning();
}
void MessageWritingDict()
{
    fprintf(stderr, "Writing dictionary... ");
    NewBeginning();
}

void MessageLoadingItem(const std::string& header)
{
    if(header.size() > 50)
        SectionMessage(header.substr(0,23)+"..."+header.substr(header.size()-23));
    else
        SectionMessage(header);
}
void MessageZSection(const std::string& header)
{
    SectionMessage(header);
}
void MessageRSection(const std::string& header)
{
    SectionMessage(header);
}
void MessageLSection(const std::string& header)
{
    SectionMessage(header);
}
void MessageDSection(const std::string& header)
{
    SectionMessage(header);
}
void MessageSSection(const std::string& header)
{
    SectionMessage(header);
}
void MessageESection(const std::string& header)
{
    SectionMessage(header);
}
void MessageC8Section(const std::string& header)
{
    SectionMessage(header);
}
void MessageUnknownHeader(const std::string& header)
{
    BeginError();
    fprintf(stderr, "Unknown header '%s'...", header.c_str());
    NewBeginning();
}
void MessageInvalidLabelChar(wchar_t c, unsigned label, const std::string& /*header*/)
{
    BeginError();
    fprintf(stderr,
            "$%X: Got char '%c', invalid is (in label)!", label, c);
    NewBeginning();
}
void MessageInvalidLabelChar(wchar_t c, const std::string& label, const std::string& /*header*/)
{
    BeginError();
    fprintf(stderr,
           "$%s: Got char '%c', expected ':' (in label)!", label.c_str(), c);
    NewBeginning();
}
void MessageWorking()
{
    Working();
}
void MessageUnexpected(const std::wstring& unexpected)
{
    BeginError();
    fprintf(stderr, "Unexpected data '%s'", WstrToAsc(unexpected).c_str());
    NewBeginning();
}
void MessageDone()
{
    SectionMessage("done.");
    fprintf(stderr, "\n");
}
void MessageSymbolIgnored(const std::wstring& symbol)
{
    BeginError();
    fprintf(stderr,
        "Warning: Symbol '%s' wouldn't work in different typefaces!\n"
        "         Thus not generating.",
            WstrToAsc(symbol).c_str());
    NewBeginning();
}
void MessageTFStartError(const std::wstring& tf)
{
    BeginError();
    fprintf(stderr, "Error: Started typeface with '%s' when one was already active.",
            WstrToAsc(tf).c_str());
    NewBeginning();
}
void MessageTFEndError(const std::wstring& tf)
{
    BeginError();
    fprintf(stderr, "Error: Ended typeface with '%s' when none was active.",
            WstrToAsc(tf).c_str());
    NewBeginning();
}
void MessageTooLongText(const ctstring& input, const ctstring& output)
{
    BeginError();
    fprintf(stderr,
        "Error: Too long text\n"
        "In:  %s\n"
        "Out: %s\n"
        "... ",
        DispString(input).c_str(),
        DispString(output).c_str());
    NewBeginning();
}

void MessageModuleWithoutAddress(const std::string& name)
{
    BeginError();
    fprintf(stderr, "O65 linker: Module %s is still without address\n", name.c_str());
    NewBeginning();
}

void MessageUndefinedSymbol(const std::string& name, const std::string& modname)
{
    BeginError();
    fprintf(stderr, "O65 linker: %s: Symbol '%s' still undefined\n",
        modname.c_str(), name.c_str());
    NewBeginning();
}

void MessageDuplicateDefinition(const std::string& name, unsigned nmods, unsigned ndefs)
{
    BeginError();
    fprintf(stderr, "O65 linker: Symbol '%s' defined in %u module(s) and %u global(s)\n",
        name.c_str(), nmods, ndefs);
    NewBeginning();
}
void MessageDuplicateDefinitionAt(const std::string& name, const std::string& who, const std::wstring& where)
{
    BeginError();
    fprintf(stderr,
        "O65 linker: ERROR:"
        " Symbol \"%s\" defined by object \"%s\""
        " is already present in object \"%s\"\n",
        name.c_str(),
        who.c_str(),
        where.c_str());
    NewBeginning();
}

void MessageDuplicateDefinitionAs(const std::string& name, unsigned value, unsigned newvalue)
{
    BeginError();
    fprintf(stderr,
         "O65 linker: Error: %s previously defined as %X,"
         " can not redefine as %X\n",
        name.c_str(), value, newvalue);
    NewBeginning();
}

void MessageUndefinedSymbols(const std::string& modname, unsigned n)
{
    BeginError();
    fprintf(stderr, "O65 linker: %s: Still %u undefined symbol(s)\n",
        modname.c_str(), n);
    NewBeginning();
}

void MessageUnresolvedSymbol(const std::string& name)
{
    BeginError();
    fprintf(stderr,
        "O65 linker: ERROR: Unresolved symbol: %s\n",
        name.c_str());
    NewBeginning();
}

void MessageUnusedSymbol(const std::string& name)
{
    BeginError();
    fprintf(stderr,
        "O65 linker: Warning: Unused symbol: %s\n",
        name.c_str());
    NewBeginning();
}
