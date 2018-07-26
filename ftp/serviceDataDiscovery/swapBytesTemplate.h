/*
 */

/* 
 * File:   swapBytesTemplate.h
 * Author: mcp
 *
 * Created on February 5, 2016, 11:26 PM
 */

#ifndef SWAPBYTESTEMPLATE_H
#define SWAPBYTESTEMPLATE_H

template<typename T>
struct swap_bytes<T, 2>
{
    inline T operator()(T val)
    {
        return ((((val) >> 8) & 0xff) | (((val) & 0xff) << 8));
    }
};

template<typename T>
struct swap_bytes<T, 4>
{
    inline T operator()(T val)
    {
        return ((((val) & 0xff000000) >> 24) |
                (((val) & 0x00ff0000) >>  8) |
                (((val) & 0x0000ff00) <<  8) |
                (((val) & 0x000000ff) << 24));
    }
};

template<>
struct swap_bytes<float, 4>
{
    inline float operator()(float val)
    {
        uint32_t mem =swap_bytes<uint32_t, sizeof(uint32_t)>()(*(uint32_t*)&val);
        return *(float*)&mem;
    }
};

template<typename T>
struct swap_bytes<T, 8>
{
    inline T operator()(T val)
    {
        return ((((val) & 0xff00000000000000ull) >> 56) |
                (((val) & 0x00ff000000000000ull) >> 40) |
                (((val) & 0x0000ff0000000000ull) >> 24) |
                (((val) & 0x000000ff00000000ull) >> 8 ) |
                (((val) & 0x00000000ff000000ull) << 8 ) |
                (((val) & 0x0000000000ff0000ull) << 24) |
                (((val) & 0x000000000000ff00ull) << 40) |
                (((val) & 0x00000000000000ffull) << 56));
    }
};


#endif /* SWAPBYTESTEMPLATE_H */

