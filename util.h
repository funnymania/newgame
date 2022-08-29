#ifndef UTIL_H
#define UTIL_H

static void ParseWavHeader(u8* data, AudioAsset* result) 
{
    u8* scan = data;
    
    // scan until 'fmt'.
    while(true) {
        if (*scan == 'f' && *(scan + 1) == 'm' && *(scan + 2) == 't') {
            // scan += 8;
            break;
        }

        scan += 1;
    }

        scan += 1;
        scan += 1;
        scan += 1;
        scan += 1;
        scan += 1;
        scan += 1;
        scan += 1;
        scan += 1;

        // format tag is next 2 bytes.
        scan += 1;
        scan += 1;


    // note: All of these fields are represented as little endian.
    u32 tmp = 0;

    // next 2 bytes are channels
    result->channels += *scan;
    scan += 1;

    tmp = *scan << 8;
    result->channels += tmp;
    scan += 1;


    // after four bytes are sample rate
    result->sample_rate += *scan;
    scan += 1;

    tmp = *scan << 8;
    result->sample_rate += tmp;
    scan += 1;

    tmp = *scan << 16;
    result->sample_rate += tmp;
    scan +=1;

    tmp = *scan << 24;
    result->sample_rate += tmp;
    scan += 1;

    // after four bytes are avgbitrate.
    // after two bytes are the alignment
    
    scan += 6;

    // after four bytes are the bit.
    u32 bits;

    result->bits += *scan;
    scan += 1;

    tmp = *scan << 8;
    result->bits += tmp;
    scan += 1;
}

std::string FromCString(char* c_string) {
    std::string res;
    u8* ptr = nullptr;
    while (*ptr != '\0') {
        res += *ptr; 
        ptr += 1;
    }
    
    return(res);
}

// Copies into dest from src, moving backwards through src a size amount, instead of forwards.
void reverse_memcpy(void* dest, void* src, u32 size) 
{
    u8* dest_tmp = (u8*)dest;
    u8* src_tmp = (u8*)src;
    for (int counter = 0; counter < size; counter += 1) {
        // *dest = *src;
        // src -= 1;
        // dest += 1;
        // dest_tmp[0 + counter] = src_tmp[0 - counter];
        dest_tmp[0] = *src_tmp;
        src_tmp -= 1;
        dest_tmp += 1;
    }
}
#endif
