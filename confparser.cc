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
 private:
    const CharIStream& operator=(const CharIStream&);
    CharIStream(const CharIStream&);
};

ConfParser::CharIStream::CharIStream(FILE *f):
    conv(getcharset()),
    fp(f),
    cache(), cacheptr(0),
    nextChar(getC()), line(1)
{
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
            char Buf[512];
            size_t n = fread(Buf, 1, sizeof Buf, fp);
            if(n == 0) break;
            
            cache += conv.puts(std::string(Buf, n));
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
    return c1 == (unsigned char)c2;
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
        throw invalid_section(sectionName);
    }
    
    return iter->second;
}


const ConfParser::Field&
ConfParser::Section::operator[](const std::string& fieldName) const
{
    FieldMap::const_iterator iter = fields.find(fieldName);
    if(iter == fields.end())
    {
        throw invalid_field(SectName, fieldName);
    }
    return iter->second;
}

unsigned ConfParser::Field::IField() const
{
    if(!elements.empty()) return elements[0].IField;
    return 0;
}

double ConfParser::Field::DField() const
{
    if(!elements.empty()) return elements[0].DField;
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
                if(is.equal(c, '\\'))
                {
                    c = is.getChar();
                    if(is.equal(c, 'n')) c = '\n';
                    else if(is.equal(c, 't')) c = '\t';
                    else if(is.equal(c, 'r')) c = '\r';
                }
                else
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
        else if(is.isDigit(c) || is.equal(c, '$'))
        {
            bool isHex = false;
            if(is.equal(c, '0') && is.equal(is.peekChar(), 'x'))
            {
                is.getChar(); c = is.getChar();
                isHex = true;
            }
            else if(is.equal(c, '$'))
            {
                c = is.getChar();
                isHex = true;
            }

            Field::Element element;
            element.IField = 0;
            element.DField = 0;
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
            element.DField = element.IField;
            if(is.peekChar() == '%')
            {
                element.DField /= 100.0;
                is.getChar();
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
            element.DField = 1;
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
            element.DField = 0;
            field.elements.push_back(element);
        }
        else
        {
            fprintf(stderr, "Syntax error in config line %u, expected a value but got '%c'\n",
                is.getLineNumber(),
                (char)c);
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
        
        // Section name?
        if(is.equal(c, '[')) break;

        // Comment line:
        if(is.equal(c, '#'))
        {
            is.getChar();
            is.getLine();
            continue;
        }

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
            fprintf(stderr, "Syntax error in config line %u, expected field name but got '%c'\n",
                is.getLineNumber(), (char)c);
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
