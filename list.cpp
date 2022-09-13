#include "list.h"

template <typename T>
void AddToList(List<T>* list, T new_t, GameMemory* memory) 
{
    if (list->array == 0) {
        list->array = (T*)memory->next_available;    
        *(list->array) = new_t;

        list->size += 1;

        memory->permanent_storage_remaining -= sizeof(T);
        memory->next_available += sizeof(T);
    } else {
        T* tmp = list->array + list->size;
        *tmp = new_t;

        list->size += 1;

        memory->permanent_storage_remaining -= sizeof(T);
        memory->next_available += sizeof(T);
    }
}

template <typename T>
T* Get(List<T> list, u64 index) 
{
    return(list.array + index);
}
