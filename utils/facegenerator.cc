#include <cstdio>
#include <cmath>
#include "tgaimage.hh"

namespace
{
    /* Cursive settings: */
    const double slant_0  = 1.5;
    const double slant_11 = -0.8;
    
    /* Bold settings: */
    const double stretch = 1.2;
    const double leak = 0.3;

    class value_t
    {
        double x, y;
    public:
        value_t() : x(0), y(0)
        {
        }
        
        value_t(double xx, double yy): x(xx), y(yy)
        {
        }

        /* Take a proportion (0..1) */
        const value_t operator* (double factor) const
        {
            return value_t(x*factor, y*factor);
        }
        
        value_t& operator+= (const value_t& b)
        {
            x += b.x;
            y += b.y;
            return *this;
        }
        
        /* Make a difference */
        const double diff(const value_t& b) const
        {
            double dist_x = (x - b.x);
            double dist_y = (y - b.y);
            
            return max(fabs(dist_x), fabs(dist_y));
            //return dist_x*dist_x + dist_y*dist_y;
        }
    };
    
    const value_t palette[6] =
    {
        value_t(  0,  0),  /* unused */
        value_t(  0,  0),  /* background */
        value_t( -1,-.5),  /* edges */
        value_t(-.9,  0),  /* corners */
        value_t(  1, -1),  /* paint */
        value_t(  0, .5)   /* empty */
    };
    
    const char *const colors[6] =
    {
        "\33[0;1;31m",
        "\33[0;1;30m",
        "\33[0;30m",
        "\33[0;31m",
        "\33[0;1;31m",
        "\33[0;1;37m"
    };

    void DispBox(const vector<char> &box)
    {
        for(unsigned y=0; y<12; ++y)
        {
            for(unsigned x=0; x<12; ++x)
            {
                unsigned p = y*12 + x;
                
                int c = box[p];
                
                printf("%s#", colors[c]);
            }
            printf("\33[0;37m\n");
        }
    }

    vector<char> MakeCursive(const vector<char> &box)
    {
        vector<value_t> box2v(12 * 12);
        
        DispBox(box);
        
        for(unsigned y=0; y<12; ++y)
        {
            double slant = y * (slant_11 - slant_0) / 11.0 + slant_0;
            
            fprintf(stderr, "Slant[%u]=%g\n", y, slant);
            
            for(int x = -3; x < 13; ++x)
            {
                unsigned p = y*12 + x;
                
                int c;
                
                if(x < 0 || x >= 12)
                    c = 5;
                else
                {
                    c = box[p];
                    if(c == 1) c = 5;
                }
                
                const value_t value = palette[c];
                
                double new_x = x + slant;
                
                int xoffs = (int)floor(new_x);
                
                double offset = new_x - xoffs;
                
                double proportion_next = offset;
                double proportion_here = 1 - proportion_next;
                
                /*
                fprintf(stderr, "x=%d,new_x=%g, xoffs=%d, here=%g, next=%g\n",
                    x, new_x, xoffs, proportion_here, proportion_next);
                */
                
                if(xoffs >= 0 && xoffs < 12)
                    box2v[y*12 + xoffs] += value * proportion_here;
                ++xoffs;
                if(xoffs >= 0 && xoffs < 12)
                    box2v[y*12 + xoffs] += value * proportion_next;
            }
        }

        vector<char> box2(12 * 12, 5);

        unsigned maxwidth=0;
        for(unsigned y=0; y<12; ++y)
        {
            unsigned width=0;
            for(unsigned x=0; x<12; ++x)
            {
                unsigned p = y*12 + x;
                const value_t value = box2v[p];

                double best_diff = 0;
                int best_diff_index = 0;
                bool first = true;
                for(int index=0; index<6; ++index)
                {
                    if(index == 0 || index == 1) continue;
                    
                    double diff = palette[index].diff(value);
                    if(diff < 0) diff = -diff;
                    if(first || diff < best_diff)
                    {
                        best_diff = diff;
                        first = false;
                        best_diff_index = index;
                    }
                }
                box2[p] = best_diff_index;
                if(best_diff_index != 5 && best_diff_index != 1)
                    width = x+1;
            }
            if(width > maxwidth) maxwidth = width;
        }
        for(unsigned y=0; y<12; ++y)
        {
            for(unsigned x=0; x<maxwidth; ++x)
            {
                unsigned p = y*12 + x;
                if(box2[p] == 5) box2[p] = 1;
            }
        }
        
        DispBox(box2);
        
        return box2;
    }

    vector<char> MakeBold(const vector<char> &box)
    {
        vector<value_t> box2v(12 * 12);
        
        DispBox(box);
        
        for(unsigned y=0; y<12; ++y)
        {
            for(int x = 0; x < 12; ++x)
            {
                unsigned p = y*12 + x;
                
                int c = box[p];
                if(c == 1) c = 5;
                
                const value_t value = palette[c];

                double new_x = x * stretch;
                
                int xoffs = (int)floor(new_x);
                
                double offset = new_x - xoffs;
                
                double values[3];
                values[0] = 1 - offset;
                values[1] = offset;
                values[2] = 0;
                
                values[0] *= stretch;
                values[1] *= stretch;
                
                if(c == 4)
                {
                    values[2] += values[1]*leak;
                    values[1] += values[0]*leak;
                }
                
                for(int offs=0; offs<3; ++offs, ++xoffs)
                {
                    if(xoffs >= 0 && xoffs < 12)
                        box2v[y*12 + xoffs] += value * values[offs];
                }
            }
        }

        vector<char> box2(12 * 12, 5);

        unsigned maxwidth=0;
        for(unsigned y=0; y<12; ++y)
        {
            unsigned width=0;
            for(unsigned x=0; x<12; ++x)
            {
                unsigned p = y*12 + x;
                const value_t value = box2v[p];

                double best_diff = 0;
                int best_diff_index = 0;
                bool first = true;
                for(int index=0; index<6; ++index)
                {
                    if(index == 0 || index == 1) continue;
                    
                    double diff = palette[index].diff(value);
                    if(diff < 0) diff = -diff;
                    if(first || diff < best_diff)
                    {
                        best_diff = diff;
                        first = false;
                        best_diff_index = index;
                    }
                }
                box2[p] = best_diff_index;
                if(best_diff_index != 5 && best_diff_index != 1)
                    width = x+1;
            }
            if(width > maxwidth) maxwidth = width;
        }
        for(unsigned y=0; y<12; ++y)
        {
            for(unsigned x=0; x<maxwidth; ++x)
            {
                unsigned p = y*12 + x;
                if(box2[p] == 5) box2[p] = 1;
            }
        }
        
        DispBox(box2);
        
        return box2;
    }
}

int main(void)
{
    TGAimage font12("ct16fn.tga");
    font12.setboxsize(12, 12);
    font12.setboxperline(32);
    
    unsigned maxbox = font12.getboxcount();
    unsigned xdim = 32;
    unsigned ydim = (maxbox + xdim - 1) / xdim;
    unsigned xpixdim = xdim*12 + (xdim+1);
    unsigned ypixdim = ydim*12 + (ydim+1);
    TGAimage newfont12(xpixdim, ypixdim, 0);
    
    for(unsigned a=0; a<maxbox; ++a)
    {
        vector<char> box = font12.getbox(a);
        
        unsigned t=a;
        unsigned ypos = (t/xdim) * (12+1) + 1;
        for(unsigned p=0,y=0; y<12; ++y,++ypos)
        {
            unsigned xpos = (t%xdim) * (12+1) + 1;
            for(unsigned x=0; x<12; ++x)
                newfont12.PSet(xpos++, ypos, box[p++]);
        }
    }
    
    for(unsigned a=0xA0; a<0xFF; ++a)
    {
        vector<char> newbox = MakeCursive(font12.getbox(a));
        
        const unsigned cursive_offset = 0x120 - 0xA0;
         
        unsigned t = a+cursive_offset;
        unsigned ypos = (t/xdim) * (12+1) + 1;
        for(unsigned p=0,y=0; y<12; ++y,++ypos)
        {
            unsigned xpos = (t%xdim) * (12+1) + 1;
            for(unsigned x=0; x<12; ++x)
                newfont12.PSet(xpos++, ypos, newbox[p++]);
        }
    }

/*
    for(unsigned a=0xA0; a<0xFF; ++a)
    {
        vector<char> newbox = MakeBold(font12.getbox(a));
        
        const unsigned bold_offset = 0x180 - 0xA0;
         
        unsigned t = a + bold_offset;
        unsigned ypos = (t/xdim) * (12+1) + 1;
        for(unsigned p=0,y=0; y<12; ++y,++ypos)
        {
            unsigned xpos = (t%xdim) * (12+1) + 1;
            for(unsigned x=0; x<12; ++x)
                newfont12.PSet(xpos++, ypos, newbox[p++]);
        }
    }
*/
    
    newfont12.Save("ct16fn_.tga", TGAimage::pal_6color);
    
    return 0;
}
