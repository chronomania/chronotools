#include <cstdio>
#include <vector>
#include <string>

#include "assemble.hh"
#include "precompile.hh"

#include <argh.hh>

int main(int argc, const char* const *argv)
{
    ParamHandler Argh;
    
    Argh.AddLong("help",       'h').SetBool().SetDesc("This help");
    Argh.AddLong("version",    500).SetBool().SetDesc("Displays version information");
    Argh.AddString('o').SetDesc("Place the output into <file>", "<file>");
    Argh.AddBool('E').SetDesc("Preprocess only");
    Argh.AddBool('c').SetDesc("Ignored for gcc-compatibility");

    Argh.AddLong("submethod", 501).SetString().
        SetDesc("Select subprocess method: temp,thread,pipe", "<method>");
    Argh.AddLong("temps", 502).SetBool().SetDesc("Short for --submethod=temp");
    Argh.AddLong("pipes", 503).SetBool().SetDesc("Short for --submethod=pipe");
    Argh.AddLong("threads", 504).SetBool().SetDesc("Short for --submethod=thread");
    
    Argh.StartParse(argc, argv);
    
    bool assemble = true;
    std::vector<std::string> files;
    
    std::FILE *output = NULL;

    for(;;)
    {
        int c = Argh.GetParam();
        if(c == -1)break;
        switch(c)
        {
            case 500: //version
                printf(
                    "%s %s\n"
                    "Copyright (C) 1992,2003 Bisqwit (http://iki.fi/bisqwit/)\n"
                    "This is free software; see the source for copying conditions. There is NO\n"
                    "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n",
                    Argh.ProgName().c_str(), VERSION
                      );
                return 0;
            case 501: //submethod
            {
                const std::string method = Argh.GetString();
                if(method == "temp" || method == "temps")
                    UseTemps();
                else if(method == "thread" || method == "threads")
                    UseThreads();
                else if(method == "pipe" || method == "pipes"
                     || method == "fork" || method == "forks")
                    UseFork();
                else
                {
                    std::fprintf(stderr, "Error: --method requires 'pipe', 'thread' or 'temp'\n");
                    return -1;
                }
                break;
            }
            case 502: if(Argh.GetBool()) UseTemps(); break;
            case 503: if(Argh.GetBool()) UseFork(); break;
            case 504: if(Argh.GetBool()) UseThreads(); break;
            case 'h':
                std::printf(
                    "65816 assembler\n"
                    "\nUsage: %s [<option> [<...>]] <file> [<...>]\n"
                    "\nAssembles 65816 code.\n"
                    "\nOptions:\n",
                    Argh.ProgName().c_str());
                Argh.ListOptions();
                std::printf("\nNo warranty whatsoever.\n");
                return 0;
            case 'E':
                assemble = !Argh.GetBool();
                break;
            case 'o':
            {
                const std::string& filename = Argh.GetString();
                if(output)
                {
                    std::fclose(output);
                }
                output = std::fopen(filename.c_str(), "wb");
                if(!output)
                {
                    std::perror(filename.c_str());
                    return -1;
                }
                break;
            }
            default:
                files.push_back(Argh.GetString());
        }
    }
    if(!Argh.ok())return -1;
    if(!files.size())
    {
        fprintf(stderr, "Error: Assemble what? See %s --help\n",
            Argh.ProgName().c_str());
        return -1;
    }
    
    Object obj;
    
    /*
     *   TODO:
     *        - Read input
     *        - Write object
     *        - Verbose errors
     *        - Verbose errors
     *        - Verbose errors
     *        - Verbose errors
     *        - Verbose errors
     */
    
    for(unsigned a=0; a<files.size(); ++a)
    {
        std::FILE *fp = NULL;
        
        const std::string& filename = files[a];
        if(filename != "-" && !filename.empty())
        {
            fp = fopen(filename.c_str(), "rt");
            if(!fp)
            {
                std::perror(filename.c_str());
                continue;
            }
        }
    
        obj.StartScope();
        obj.SelectTEXT();
        if(assemble)
            PrecompileAndAssemble(fp ? fp : stdin, obj);
        else
            Precompile(fp ? fp : stdin, output ? output : stdout);
        obj.EndScope();
        
        if(fp)
            std::fclose(fp);
    }
    
    if(assemble)
    {
        obj.Link();
    
        obj.WriteOut(output ? output : stdout);
        obj.Dump();
    }
    
    if(output) fclose(output);
    
    return 0;
}
