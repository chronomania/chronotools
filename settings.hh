// conjugate.cc:
namespace
{
    const char functionfn[]            = "ct.code";
    const unsigned char AllowedBytes[] = {0xFF,0xFE,0xFC,0xFB,0xFA};
}

// ctcset.cc:
namespace
{
    // Document character set
    const char CharSet[] = "iso-8859-1";

    // Map of chrono symbols to document set
    const char Font16[] =
     // This is supposed to be encoded in CharSet, which may be multibyte.
     //                       indx count
     "¶¶¶¶¶¶¶¶" "¶¶¶¶¶¶¶¶"   // 00 100
     "¶¶¶¶¶¶¶¶" "¶¶¶¶¶¶¶¶"   // 10 F0
     "¶¶¶¶¶¶¶¶" "¶¶¶¶¶¶¶¶"   // 20 E0
     "¶¶¶¶¶¶¶¶" "¶¶¶¶¶¶¶¶"   // 30 D0
     "¶¶¶¶¶¶¶¶" "¶¶¶¶¶¶¶¶"   // 40 C0
     "¶¶¶¶¶¶¶¶" "¶¶¶¶¶¶¶¶"   // 50 B0
     "¶¶¶¶¶¶¶¶" "¶¶¶¶¶¶¶¶"   // 60 A0
     "¶¶¶¶¶¶¶¶" "¶¶¶¶¶¶¶¶"   // 70 90
     "¶¶¶¶¶¶¶¶" "¶¶¶¶¶¶¶¶"   // 80 80
     "¶¶¶¶¶¶¶¶" "¶¶¶¶¶¶¶¶"   // 90 70
     "ABCDEFGH" "IJKLMNOP"   // A0 60
     "QRSTUVWX" "YZabcdef"   // B0 50
     "ghijklmn" "opqrstuv"   // C0 40
     "wxyz0123" "456789ÅÄ"   // D0 30
     "Ö«»:-()'" ".,åäöé¶ "   // E0 20  EE=musicsymbol
     "¶¶%É=&+#" "!?¶¶¶/¶_";  // F0 10  F0=heartsymbol, F1=..., F2=originally infinity

    // Default in Chrono Trigger is 0x60 (0xA0-0xFF).
    const unsigned char Num_Characters       = 0x7A;
}

// ctinsert.cc:
namespace
{
    const char font8fn[]                = "ct8fn.tga";
    const char font12fn[]               = "ct16fn.tga";
    const char scriptfn[]               = "ct.txt";
    
    const char patchfile_hdr[]          = "ctpatch-hdr.ips";
    const char patchfile_nohdr[]        = "ctpatch-nohdr.ips";
    
    // in ips files:
    const unsigned MaxHunkSize = 20000;
}

// compiler.cc:
namespace
{
    const char LoopHelperName[] = "$LoopHelper$";
    const char OutcHelperName[] = "$OutcHelper$";
}

// readin.cc:
namespace
{
    // Ilmoita missä wrapattiin
    const bool warn_wraps   = false;
    
    // Tarkista, aiheuttiko wrap ongelmia
    const bool verify_wraps = true;
}

// dictionary.cc:
namespace
{
    // Rebuild dictionary?
    const bool rebuild_dict             = true;
    
    // If rebuild, produces 50% less effecient dictionary in 90% less time
    const bool dictbuild_reallyquick    = false;
    
    // Sort dictionary before applying?
    const bool sort_dictionary          = false;
    
    // Maximum length of substring in generated dictionary
    const unsigned MaxDictWordLen       = 16;

    // Maximum recursion in generated dictionary
    const unsigned MaxDictReuseCount    = 0;
    
    // Maximum spaces in generated dictionary word (when nonspaces present)
    const unsigned DictMaxSpacesPerWord = 2;
}
