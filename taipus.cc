#include <string>
#include <iostream>
using namespace std;

/*
 * This program is an illustration of taipus.rb in C++.
 * It attempts to implement the program so simply that
 * it should be easy to translate to asm.
 */

typedef unsigned char byte;

static byte Names[] = "Matti\0Marle\0Lucca\0Robo\0\0Frog\0\0Ayla\0\0Magus\0Epoch\0";

unsigned GetPointer(byte char_id)
{
    switch(char_id)
    {
        case 0x13: return 0; // Crono
        case 0x14: return 6; // Marle
        case 0x15: return 12;// Lucca
        case 0x16: return 18;// Robo
        case 0x17: return 24;// Frog
        case 0x18: return 30;// ayla
        case 0x19: return 36;// Magus
        default:
        case 0x20: return 42;// Epoch
    }
}

bool IsFront(byte char_id)
{
    unsigned ptr = GetPointer(char_id);
    byte hint1= 0;
    byte hint2= 0;
    byte hint3= 0;
    for(;;)
    {
        byte c = Names[ptr++];
        if(!c) break;
        
        switch(c)
        {
            case 'a': case 'o': case 'u':
            case 'A': case 'O': case 'U':
            case '0': case '2': case '3': case '6': case '8':
                hint1 = 1;
                hint3 = 1;
                break;

            case 'ä': case 'ö': case 'y':
            case 'Ä': case 'Ö': case 'Y':
            case '1': case '4': case '5': case '7': case '9':
                hint1 = 2;
                hint3 = 1;
                break;

            case 'e': case 'i': case 'é':
            case 'E': case 'I': case 'É':
                hint2 = 1;
                hint3 = 1;
                break;

            case 'h': case 'k': case 'q': case 'å':
            case 'H': case 'K': case 'Q': case 'Å':
                hint3 = 0;
                break;
        }
    }
    if(hint1 > 0) return hint1 == 2;
    if(hint2 > 0) return true;
    return hint3 == 1;
}

byte Length(byte char_id)
{
    unsigned ptr = GetPointer(char_id);
    byte hint= 0;
    for(;;)
    {
        byte c = Names[ptr++];
        if(!c) break;
        
        ++hint;
    }
    return hint;
}

byte LastChar3(byte char_id)
{
    unsigned ptr = GetPointer(char_id);
    byte hint1=0, hint2=0, hint3=0;
    for(;;)
    {
        byte c = Names[ptr++];
        if(!c) break;
        hint3 = hint2;
        hint2 = hint1;
        hint1 = c;
    }
    return hint3;
}
byte LastChar2(byte char_id)
{
    unsigned ptr = GetPointer(char_id);
    byte hint1=0, hint2=0;
    for(;;)
    {
        byte c = Names[ptr++];
        if(!c) break;
        hint2 = hint1;
        hint1 = c;
    }
    return hint2;
}
byte LastChar1(byte char_id)
{
    unsigned ptr = GetPointer(char_id);
    byte hint1= 0;
    for(;;)
    {
        byte c = Names[ptr++];
        if(!c) break;
        
        hint1 = c;
    }
    return hint1;
}

byte IsVowel(byte merkki)
{
    switch(merkki)
    {
        case 'a': case 'o': case 'u':
        case 'A': case 'O': case 'U':
        case 'ä': case 'ö': case 'y':
        case 'Ä': case 'Ö': case 'Y':
        case 'e': case 'i': case 'é':
        case 'E': case 'I': case 'É':
        case 'å': case 'Å':
        case '0': case '1': case '2':
        case '3': case '4': case '5':
        case '6':
            return true;
    }
    return false;
}

bool IsKPT(byte merkki)
{
    switch(merkki)
    {
        case 'k': case 'K':
        case 'p': case 'P':
        case 't': case 'T':
            return true;
    }
    return false;
}

bool IsAbbrev(byte char_id)
{
    unsigned ptr = GetPointer(char_id);
    byte hint1= 0;
    byte hint2= 0;
    byte hint3= 0;
    for(;;)
    {
        byte c = Names[ptr++];
        if(!c) break;
        
        ++hint1;
        
        switch(c)
        {
            case 'a': case 'o': case 'u':
            case 'A': case 'O': case 'U':
            case 'ä': case 'ö': case 'y':
            case 'Ä': case 'Ö': case 'Y':
            case 'e': case 'i': case 'é':
            case 'E': case 'I': case 'É':
            case 'å': case 'Å':
                hint2 = 1;
                hint3 = 1;
                break;

            case '0': case '2': case '3': case '6': case '8':
            case '1': case '4': case '5': case '7': case '9':
                if(hint3) hint2 = 0;
                // passthru
            default:
                hint3 = 0;
                break;
        }
    }
    if(hint1 == 1) return true;
    return hint2 == 0;
}

bool IsEs(byte char_id)
{
    byte hint = LastChar2(char_id);
    if(!IsVowel(hint))
        return false;
    
    hint = LastChar1(char_id);
    return hint == 's' || hint == 'S';
}

bool EndWithVowel(byte char_id)
{
    byte hint = LastChar1(char_id);
    return IsVowel(hint);
}

bool DoubleHard(byte char_id)
{
    if(!EndWithVowel(char_id)) return false;
    
    byte hint2 = LastChar2(char_id);
    byte hint3 = LastChar3(char_id);
    
    if(hint2 != hint3) return false;
    
    return IsKPT(hint2);
}

bool AkiEnd(byte char_id)
{
    byte hint;
    
    hint = LastChar3(char_id);
    if(hint != 'a' && hint != 'ä'
    && hint != 'A' && hint != 'Ä') return false;
    
    hint = LastChar2(char_id);
    if(!IsKPT(hint)) return false;
    
    hint = LastChar1(char_id);
    if(hint != 'i' && hint != 'I') return false;
    return true;
}

void OutByte(byte c)
{
    cout << ((char)c) << flush;
}
void OutWord(byte char_id, byte length)
{
    unsigned ptr = GetPointer(char_id);
    while(length > 0)
    {
        byte c = Names[ptr++];
        OutByte(c);
        --length;
    }
}

void HardStem(byte char_id)
{
    byte length = Length(char_id);
    
    if(IsAbbrev(char_id))
    {
        OutWord(char_id, length);
        OutByte(':');
        byte hint = LastChar1(char_id);
        switch(hint)
        {
            case 'f': case 'l': case 'm': case 'n':
            case 'r': case 's': case 'w': case 'x':
            case 'F': case 'L': case 'M': case 'N':
            case 'R': case 'S': case 'W': case 'X':
            case '4': case '7': case '9':
                OutByte('ä'); break;
            case 'z': case 'Z':
                OutByte('a'); break;
            case '0': case '1': case '2': case '3':
            case '5': case '6': case '8':
                break;
            default:
                OutByte('t');
        }
        return;
    }
    if(IsEs(char_id))
    {
        OutWord(char_id, length);
        OutByte('t');
        return;
    }
    if(!EndWithVowel(char_id))
    {
        OutWord(char_id, length);
        OutByte('i');
        return;
    }
    if(AkiEnd(char_id))
    {
        OutWord(char_id, length-1);
        OutByte('e');
        return;
    }
    OutWord(char_id, length);
}

void SoftStem(byte char_id)
{
    byte length = Length(char_id);
    
    if(IsAbbrev(char_id))
    {
        OutWord(char_id, length);
        OutByte(':');
        return;
    }
    if(IsEs(char_id))
    {    
        OutWord(char_id, length-1);
        OutByte('k');
        OutByte('s');
        OutByte('e');
        return;
    }
    if(!EndWithVowel(char_id))
    {
        OutWord(char_id, length);
        OutByte('i');
        return;
    }
    if(DoubleHard(char_id))
    {
        OutWord(char_id, length-2);
        byte hint = LastChar1(char_id);
        OutByte(hint);
        return;
    }
    if(AkiEnd(char_id))
    {
        OutWord(char_id, length-2);
        OutByte('e');
        return;
    }
    OutWord(char_id, length);
}

void Out_A(byte char_id)
{
    if(IsFront(char_id))
        OutByte('ä');
    else
        OutByte('a');
}

void Do_N(byte char_id)
{
    SoftStem(char_id);
    OutByte('n');
}

void Do_A(byte char_id)
{
    HardStem(char_id);
    Out_A(char_id);
}

void Do_LLA(byte char_id)
{
    SoftStem(char_id);
    OutByte('l');
    OutByte('l');
    Out_A(char_id);
}

void Do_LLE(byte char_id)
{
    SoftStem(char_id);
    OutByte('l');
    OutByte('l');
    OutByte('e');
}

void Do_STA(byte char_id)
{
    SoftStem(char_id);
    OutByte('s');
    OutByte('t');
    Out_A(char_id);
}

int main(void)
{
    byte ids[] = {0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x20};
    for(unsigned n=0; n<8; ++n)
    {
        byte id = ids[n];
        Do_N(id);    cout << endl;
        Do_A(id);    cout << endl;
        Do_LLE(id);  cout << endl;
        Do_LLA(id);  cout << endl;
        Do_STA(id);  cout << endl;
    }
}
