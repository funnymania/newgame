#ifndef UTIL_H
#define UTIL_H

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
