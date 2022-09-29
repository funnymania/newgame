#include "list.h"

template <typename T>
void AddToList(SimpList<T>* list, T new_t, GameMemory* memory) 
{
    if (list->array == 0) {
        list->array = (T*)memory->next_available;    
        *(list->array) = new_t;

        list->length += 1;

        memory->permanent_storage_remaining -= sizeof(T);
        memory->next_available += sizeof(T);
    } else {
        T* tmp = list->array + list->length;
        *tmp = new_t;

        list->length += 1;

        memory->permanent_storage_remaining -= sizeof(T);
        memory->next_available += sizeof(T);
    }
}

template <typename T>
T* Get(SimpList<T> list, u64 index) 
{
    return(list.array + index);
}

template <typename T>
void Set(SimpList<T> list, u64 index, T value)
{
    T* seek = list.array + index;
    *seek = value;
}

template <typename T>
SimpList<T> NewList(u64 length, GameMemory* game_memory) 
{
    SimpList<T> new_list = {};
    new_list.array = (T*)game_memory->next_available;
    T* seek = new_list.array;
    for (i32 counter = 0; counter < length; counter += 1) {
        *seek = {};
        
        game_memory->next_available += sizeof(T);
        game_memory->permanent_storage_remaining -= sizeof(T);
        seek += 1;
    }

    new_list.length = length;

    return(new_list);
}
