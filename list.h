#ifndef LIST_H
#define LIST_H

template <typename T> struct List
{
    T* array;
    u64 size;
};

template <typename T> void AddToList(List<T> list, T new_t, GameMemory* memory);

#endif
