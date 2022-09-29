#ifndef LIST_H
#define LIST_H

template <typename T> struct SimpList
{
    T* array;
    u64 length;
};

template <typename T> void AddToList(SimpList<T>* list, T new_t, GameMemory* memory);
template <typename T> T* Get(SimpList<T> list, u64 index);

#endif
