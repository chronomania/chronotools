#include <cstdio>
#include <cstring>

// These functions do not handle plural (never needed
// with names) or alphanumeric codenames.

static char NameBuf[6];

#define NAME "\1"
//   akkusatiivi = the object of action that affects the target as whole
#define ACC "\2"
//   partitiivi  = the object of action that affects the target
#define PART "\3"
//   genetiivi   = owner of something, in English usually "'s"
#define GEN "\4"
//   adessiivi   = owner/container of something, in English "has" or "in" or "on"
#define ADE "\5"
//   allatiivi   = target of something, in English sometimes as "to" or "for" preposition
#define ALLA "\6"

bool isvowel(const char c)
{
    switch(c)
    {
        case 'a': case 'e': case 'i': case 'o':
        case 'u': case 'y': case 'ä': case 'ö':
        case 'A': case 'E': case 'I': case 'O':
        case 'U': case 'Y': case 'Ä': case 'Ö':
        case 'å': case 'é': case 'Å': case 'É':
            return true;
    }
    return false;
}

/////////////////////////////////////

static void RealDispChar(char c)
{
    std::putchar(c);
}

// This function eats one character from the
// script and outputs various amount of characters.

static void DispChar(char c)
{
    static char ConjuCode = 0;
    char ConjuWord[6];
    
    // NAME: Test for conjugate
    if(c == NAME[0]) { std::strcpy(ConjuWord, NameBuf); goto ConjugateTest; }

    if(c == ACC[0]
    || c == GEN[0]
    || c == PART[0]
    || c == ADE[0]
    || c == ALLA[0]
      )
        ConjuCode = c;
    else
    {
        ConjuCode = 0;
        RealDispChar(c);
    }
    return;

ConjugateTest:
    unsigned size=0;
    while(ConjuWord[size]) ++size;
    
    if(!ConjuCode)
    {
        for(unsigned a=0; a<size; ++a)RealDispChar(ConjuWord[a]);
        return;
    }
    
    char lastchar  = ConjuWord[size > 1 ? size-1 : 0];
    char lastchar2 = ConjuWord[size > 2 ? size-2 : 0];
    char lastchar3 = ConjuWord[size > 3 ? size-3 : 0];
    
    bool hasvowel = false;
    bool yla = ConjuWord[0] != 'h' && ConjuWord[0] != 'H'
            && ConjuWord[0] != 'k' && ConjuWord[0] != 'K'
            && ConjuWord[0] != 'q' && ConjuWord[0] != 'Q'
            && ConjuWord[0] != 'z' && ConjuWord[0] != 'Z';
    
    // Description: If the last vowel of the body is a/o/u,
    // we will use the lower (ala) suffix. If the last vowel
    // of the body is ä/ö/y, we'll use the upper (ylä) suffix.
    // If the last vowel is any other vowel, check the previous
    // vowel. Otherwise we'll use the default suffix (ylä).
    for(unsigned a=size; a-->0; )
    {
        char ch = ConjuWord[a];
        if(ch == 'a' || ch == 'o' || ch == 'u'
        || ch == 'A' || ch == 'O' || ch == 'U') { yla=false; hasvowel=true; break; }
        if(ch == 'Ä' || ch == 'Ö' || ch == 'Y'
        || ch == 'ä' || ch == 'ö' || ch == 'y') { yla=true; hasvowel=true; break; }
        if(ch == '1' || ch == '4' || ch == '5'
        || ch == '7' || ch == '9') { yla=true; break; }
        if(ch == '0' || ch == '2' || ch == '3'
        || ch == '6' || ch == '8') { yla=false; break; }
    }
    

    if(ConjuCode == PART[0])
    {
        // Case "hard body"
        if(size == 1 || !hasvowel)
        {
            // Abbreviation. This isn't very good but there's no better.
            // Examples: X,a -> X:ää
            //           A,a -> A:ta
            //           P,a -> P:tä
            for(unsigned a=0; a<size; ++a)RealDispChar(ConjuWord[a]);
            RealDispChar(':');
            if(lastchar == 'f' || lastchar == 'F'
            || lastchar == 'l' || lastchar == 'L'
            || lastchar == 'm' || lastchar == 'M'
            || lastchar == 'n' || lastchar == 'N'
            || lastchar == 'l' || lastchar == 'L'
            || lastchar == 'r' || lastchar == 'R'
            || lastchar == 's' || lastchar == 'S'
            || lastchar == 'x' || lastchar == 'X')
            {
                yla = true;
                RealDispChar('ä');
            }
            else if(lastchar == 'z' || lastchar == 'Z')
            {
                yla = false;
                RealDispChar('a');
            }
            else
                RealDispChar('t');
        }
        else if((lastchar == 'S' || lastchar == 's')
              && isvowel(lastchar2))
        {
            // If the word ends with s, add "t"
            // Example: Magus,a -> Magusta
            for(unsigned a=0; a<size; ++a)RealDispChar(ConjuWord[a]);
            RealDispChar('t');
        }
        else if(isvowel(lastchar))
        {
            for(unsigned a=0; a<size; ++a)RealDispChar(ConjuWord[a]);
        }
        else
        {
            // If the word does not end with vowel, add "i"
            // Example: John,a -> Johnia
            for(unsigned a=0; a<size; ++a)RealDispChar(ConjuWord[a]);
            RealDispChar('i');
        }
    }
    else
    {
        // Case "soft body"
        if(size == 1 || !hasvowel)
        {
            // Abbreviation. This isn't very good but there's no better.
            // Example: X,lle -> X:lle
            for(unsigned a=0; a<size; ++a)RealDispChar(ConjuWord[a]);
            RealDispChar(':');
        }
        else if((lastchar == 'S' || lastchar == 's')
              && isvowel(lastchar2))
        {
            // If the word ends with vowel+s, replace s with "kse"
            // Example: Magus,lle -> Magukselle
            for(unsigned a=0; a<size-1; ++a)RealDispChar(ConjuWord[a]);
            RealDispChar('k');
            RealDispChar('s');
            RealDispChar('e');
        }
        else if(!isvowel(lastchar))
        {
            // If the word ends with non-vowel, add "i"
            // Example: John,lle -> Johnille
            for(unsigned a=0; a<size; ++a)RealDispChar(ConjuWord[a]);
            RealDispChar('i');
        }
        else if(size <= 2)
        {
            for(unsigned a=0; a<size; ++a)RealDispChar(ConjuWord[a]);
        }
        else if(isvowel(lastchar)
            && lastchar2 == lastchar3
            && (lastchar2 == 'k' || lastchar2 == 'p' || lastchar2 == 't'
             || lastchar2 == 'K' || lastchar2 == 'P' || lastchar2 == 'T')
               )
        {
            // If the word ends with hardconsonantdouble + vowel,
            // Remove one consonant.
            // Example: Mikko,lle -> Mikolle
    
            for(unsigned a=0; a<size-2; ++a)RealDispChar(ConjuWord[a]);
            RealDispChar(lastchar);
        }
        else
        {
            // Otherwise it's just that.
            // Example: Mika,lle -> Mikalle
            for(unsigned a=0; a<size; ++a)RealDispChar(ConjuWord[a]);
        }
    }

    if(ConjuCode == ACC[0])
    {
        RealDispChar('n');
    }
    else if(ConjuCode == PART[0])
    {
        RealDispChar(yla ? 'ä' : 'a');
    }
    else if(ConjuCode == GEN[0])
    {
        RealDispChar('n');
    }
    else if(ConjuCode == ADE[0])
    {
        RealDispChar('l');
        RealDispChar('l');
        RealDispChar(yla ? 'ä' : 'a');
    }
    else if(ConjuCode == ALLA[0])
    {
        RealDispChar('l');
        RealDispChar('l');
        RealDispChar('e');
    }

    ConjuCode = 0;
}

static void Hoitele(const char *s)
{
    while(*s)
        DispChar(*s++);
}

int main(int argc, const char *const *argv)
{
    std::strncpy(NameBuf, argv[1], 5);
    NameBuf[5] = 0;
    
    Hoitele
    (
        "*** Testing name '"NAME"' ***\n"
        "\n"
        "Pasi saw "NAME":\n"
        "Pasi näki "ACC NAME "\n"
        "\n"
        "Pasi thanks "NAME":\n"
        "Pasi kiittää "PART NAME "\n"
        "\n"
        NAME"'s clock is broken:\n"
        GEN NAME " kello on rikki\n"
        "\n"
        NAME" has a cat:\n"
        ADE NAME " on kissa\n"
        "\n"
        NAME" was called many times:\n"
        ALLA NAME" soitettiin monta kertaa\n"
    );
}
