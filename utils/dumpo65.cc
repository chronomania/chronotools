#include <cstdio>
#include <vector>
#include <string>

using namespace std;

#define word(x) x=fgetc(fp);x|=fgetc(fp)<<8

namespace
{
	unsigned mode;
	unsigned tbase, tlen;
	unsigned dbase, dlen;
	unsigned bbase, blen;
	unsigned zbase, zlen;
	unsigned stack;

	vector<unsigned char> text, data;

	unsigned num_undefs;
	unsigned num_extrns;
	
	FILE *fp;
	
	void Relocations(const vector<unsigned char>& segment)
	{
    	int addr = -1;
    	for(;;)
    	{
    	    int c = fgetc(fp);
    	    if(!c || c == EOF)break;
    	    if(c == 255)
    	    {
    	        addr += 254;
    	        continue;
    	    }
    	    addr += c;
    	    c = fgetc(fp);
    	    unsigned type = c & 0xE0;
    	    unsigned seg  = c & 0x07;
    	    
    	    if(seg == 0)
    	    {
    	    	unsigned apu;
    	    	word(apu);
    	    	printf("   Apu = %04X:\n", apu);
    	    }

    	    switch(type)
    	    {
    	    	case 0x20:
    	    	{
    	    		unsigned value = segment[addr] << 8;
    	    		printf("Segid=%u, type 20; 16.LO [%04X]=%04X\n", seg, addr, value);
    	    		break;
    	    	}
    	    	case 0x40:
    	    	{
    	    		unsigned apu = fgetc(fp);
    	    		unsigned value = (segment[addr] << 8) | apu;
    	    		printf("Segid=%u, type 40; 16.HI [%04X]=%04X\n", seg, addr, value);
    	    		break;
    	    	}
    	    	case 0x80:
    	    	{
    	    		unsigned value = (*(unsigned *)&segment[addr])&0xFFFF;
    	    		printf("Segid=%u, type 80; 16    [%04X]=%04X\n", seg, addr, value);
    	    		break;
    	    	}
    	    	case 0xA0:
    	    	{
    	    		unsigned value = (*(unsigned *)&segment[addr])&0xFFFFFF;
    	    		unsigned apu; word(apu);
    	    		printf("Segid=%u, type A0; 24.SEG[%04X]=%06X,%04X\n", seg, addr, value,apu);
    	    		break;
    	    	}
    	    	case 0xC0:
    	    	{
    	    		unsigned value = (*(unsigned *)&segment[addr])&0xFFFFFF;
    	    		printf("Segid=%u, type C0; 24    [%04X]=%06X\n", seg, addr, value);
    	    		break;
    	    	}
    	    	default:
    	    	{
    	    		printf("Segid=%u, type %02X at %04X\n", seg, type, addr);
    	    	}
    	    }
    	}
	}
}

int main(void)
{
	fp = fopen("a.o65", "rb");
	
	fseek(fp, 6, SEEK_SET);
	
	word(mode);
	word(tbase);  word(tlen);
	word(dbase);  word(dlen);
	word(bbase);  word(blen);
	word(zbase);  word(zlen);
	word(stack);
	
	text.resize(tlen);
	data.resize(dlen);
	
	for(;;)
	{
		unsigned len = fgetc(fp);
		if(!len) break;
		
		unsigned type = fgetc(fp);
		vector<unsigned char> value(--len);
		for(unsigned a=0; a<len; ++a)
			value[a] = fgetc(fp);

		printf("value %u: ", type); 
		for(unsigned a=0; a<len; ++a) putchar(value[a]);
		printf("\n");
	}
	
	printf("text length %u, data length %u\n", tlen, dlen);
	
	if(tlen) fread(&text[0], tlen, 1, fp);
	if(dlen) fread(&data[0], dlen, 1, fp);
	
	word(num_undefs);
	printf("%u undefs:\n", num_undefs);
	for(unsigned a=0; a<num_undefs; ++a)
	{
		string varname;
		while(int c = fgetc(fp)) varname += (char) c;
		printf("undef: %s\n", varname.c_str());
	}
	
	printf("--text--\n");
	Relocations(text);
	printf("--data--\n");
	Relocations(data);
	
	word(num_extrns);
	printf("%u externs:\n", num_extrns);
	for(unsigned a=0; a<num_extrns; ++a)
	{
		string varname;
		while(int c = fgetc(fp)) varname += (char) c;
		
		unsigned seg = fgetc(fp);
		unsigned value;
		word(value);
		
		printf("extern: %s (seg %u, value %04X)\n",
			varname.c_str(), seg, value);
	}
	
	
	fclose(fp);
}
