#include "confparser.hh"
#include "ctcset.hh"

//=======================================================================
// Abstract input stream
//=======================================================================

class ConfParser::CharIStream
{
 public:
    CharIStream(FILE *f);

    bool good();

    ucs4 peekChar() const;
    ucs4 getChar();

    const ucs4string getLine();

    bool equal(ucs4 c1, char c2) const;
    bool isSpace(ucs4 c) const;
    bool isDigit(ucs4 c) const;
    ucs4 toUpper(ucs4 c) const;

    void skipWS();

    // Convert ucs4string to std::string
    std::string toString(const ucs4string& s) const { return WstrToAsc(s); }

    unsigned getLineNumber() const;

 private:
    wstringIn conv;
    FILE *fp;
    ucs4string cache;
    unsigned cacheptr;
    ucs4 getC();
    
    ucs4 nextChar;
    unsigned line;
};

ConfParser::CharIStream::CharIStream(FILE *f):
    fp(f), cacheptr(0), line(1)
{
    conv.SetSet(getcharset());
    nextChar = getC();
}

ucs4 ConfParser::CharIStream::getC()
{
    for(;;)
    {
        if(cacheptr < cache.size())  
            return cache[cacheptr++];
        
        CLEARSTR(cache);
        cacheptr = 0;
        while(cache.empty())
        {
            int c = fgetc(fp);
            if(c == EOF)break;
            // conv.putc may generate any amount of wchars, including 0
            cache = conv.putc(c);
        }
        // So now cache may be of arbitrary size.
        if(cache.empty()) return (ucs4)EOF;      
    }
}

bool ConfParser::CharIStream::good()
{
    if(!cache.empty()) return true;
    if(nextChar != (ucs4)EOF) return true;
    return false;
}

ucs4 ConfParser::CharIStream::peekChar() const
{
    return nextChar;
}

ucs4 ConfParser::CharIStream::getChar()
{
    ucs4 retval = nextChar;
    nextChar = getC();
    if(retval == '\n') ++line;
    return retval;
}

const ucs4string ConfParser::CharIStream::getLine()
{
    ucs4string result;
    for(;;)
    {
        if(nextChar == '\r') { getChar(); continue; }
        if(nextChar == '\n') { getChar(); break; }
        result += getChar();
    }
    ++line;
    return result;
}

bool ConfParser::CharIStream::equal(ucs4 c1, char c2) const
{
    return c1 == c2;
}

bool ConfParser::CharIStream::isSpace(ucs4 c) const
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

bool ConfParser::CharIStream::isDigit(ucs4 c) const
{
    return c >= '0' && c <= '9';
}

ucs4 ConfParser::CharIStream::toUpper(ucs4 c) const
{
    if(c < 'a') return c;
    if(c > 'z') return c;
    return c + 'A' - 'a';
}

void ConfParser::CharIStream::skipWS()
{
    while(isSpace(nextChar)) getChar();
}

unsigned ConfParser::CharIStream::getLineNumber() const
{
    return line;
}

//=======================================================================
// ConfParser
//=======================================================================

const ConfParser::Section&
ConfParser::operator[](const std::string& sectionName) const
{
    SecMap::const_iterator iter = sections.find(sectionName);
    if(iter == sections.end())
    {
        fprintf(stderr, "Error: Section '%s' not present in configuration file!\n",
            sectionName.c_str());
    }
    
    return iter->second;
}


const ConfParser::Field&
ConfParser::Section::operator[](const std::string& fieldName) const
{
    FieldMap::const_iterator iter = fields.find(fieldName);
    if(iter == fields.end())
    {
        fprintf(stderr, "Error: Field '%s' not present in configuration file section '%s'!\n",
            fieldName.c_str(), SectName.c_str());
    }
    return iter->second;
}

unsigned ConfParser::Field::IField() const
{
    if(!elements.empty()) return elements[0].IField;
    return 0;
}

const ucs4string& ConfParser::Field::SField() const
{
    static const ucs4string empty;
    if(!elements.empty()) return elements[0].SField;
    return empty;
}

void ConfParser::Field::append(const Field& f)
{
    elements.insert(elements.end(), f.elements.begin(), f.elements.end());
}



bool ConfParser::ParseField(CharIStream& is, Field& field, bool mergeStrings)
{
    while(true)
    {
        ucs4 c = is.getChar();
        if(!is.good()) return false;
        if(is.equal(c, '\n')) return true;
        if(is.isSpace(c)) continue;

        if(is.equal(c, '"'))
        {
            Field::Element element;
            while(true)
            {
                c = is.getChar();
                if(!is.good()) return false;
                if(is.equal(c, '"')) break;
                element.SField += c;
            }
            if(mergeStrings && !field.elements.empty() &&
               !field.elements.back().SField.empty())
            {
                field.elements.back().SField += element.SField;
            }
            else
            {
                field.elements.push_back(element);
            }
        }
        else if(is.isDigit(c))
        {
            bool isHex = false;
            if(is.equal(c, '0') && is.equal(is.peekChar(), 'x'))
            {
                is.getChar(); c = is.getChar();
                isHex = true;
            }

            Field::Element element;
            element.IField = 0;
            while(true)
            {
                unsigned value = is.toUpper(c)-'0';
                if(isHex && value > 9) value -= 'A'-'0'-10;
                element.IField *= isHex ? 16 : 10;
                element.IField += value;

                c = is.toUpper(is.peekChar());
                if(is.isDigit(c) || (isHex && c>='A' && c<='F'))
                    c = is.getChar();
                else
                    break;
            }
            field.elements.push_back(element);
        }
        else if(is.equal(c, 't') &&
                is.equal(is.getChar(), 'r') &&
                is.equal(is.getChar(), 'u') &&
                is.equal(is.getChar(), 'e'))
        {
            Field::Element element;
            element.IField = 1;
            field.elements.push_back(element);
        }
        else if(is.equal(c, 'f') &&
                is.equal(is.getChar(), 'a') &&
                is.equal(is.getChar(), 'l') &&
                is.equal(is.getChar(), 's') &&
                is.equal(is.getChar(), 'e'))
        {
            Field::Element element;
            element.IField = 0;
            field.elements.push_back(element);
        }
        else
        {
            fprintf(stderr, "Syntax error in config line %u, expected a value\n",
                is.getLineNumber());
            return false;
        }
    }
}


void ConfParser::ParseSection(CharIStream& is, const std::string& secName)
{
    Section section;

    while(true)
    {
        is.skipWS();
        if(!is.good()) break;

        ucs4string fieldNameString;

        ucs4 c = is.peekChar();
        if(is.equal(c, '[')) break;
        if(is.equal(c, '#')) { is.getChar(); is.getLine(); continue; }

        while(true)
        {
            c = is.peekChar();
            if(is.isSpace(c) || is.equal(c, '=')) break;
            fieldNameString += is.getChar();
            if(!is.good()) return;
        }

        std::string fieldName = is.toString(fieldNameString);

        if(fieldName.empty())
        {
            fprintf(stderr, "Syntax error in config line %u, expected field name\n",
                is.getLineNumber());
            continue;
        }

        Field field;

        is.skipWS();
        c = is.peekChar();
        if(is.equal(c, '='))
        {
            is.getChar();
            if(!ParseField(is, field, true)) continue;
            section.fields[fieldName] = field;
        }
        else
        {
            if(!ParseField(is, field, false)) continue;
            section.fields[fieldName].append(field);
        }
    }

    section.SectName = secName;
    sections[secName] = section;
}


void ConfParser::Parse(FILE *fp)
{
    CharIStream is(fp);
    while(true)
    {
        is.skipWS();
        ucs4 c = is.getChar();

        if(!is.good()) break;

        // Comment line:
        if(is.equal(c, '#'))
        {
            is.getLine();
            continue;
        }

        // Expect section name:
        if(is.equal(c, '['))
        {
            ucs4string secName;
            while(true)
            {
                c = is.getChar();
                if(is.equal(c, ']')) break;
                secName += c;
            }
            ParseSection(is, is.toString(secName));
        }
        else
        {
            fprintf(stderr, "Syntax error in config line %u, expected a section marker\n",
                is.getLineNumber());
        }
    }
}

void ConfParser::Clear()
{
    sections.clear();
}
