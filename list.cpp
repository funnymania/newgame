#include "list.h"

template <typename T>
void AddToList(List<T>* list, T new_t, GameMemory* memory) 
{
    T* tmp = list->array + list->size;
    *tmp = new_t;

    list->size += 1;

    memory->permanent_storage_remaining -= sizeof(T);
    memory->next_available += sizeof(T);
}
