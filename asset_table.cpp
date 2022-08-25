#include "audio.h"

// Represents a single file.
struct StaticSoundAsset {
    char* name;
    StaticAudioStream* stream; 
    DWORD thread_id;
    StaticSoundAsset* next;
    u8* media_ptr;
};

// todo: should be a lookup table, not linked list.
// implement as an array (or, learn about cpp stuff?)
struct SoundAssetTable {
    StaticSoundAsset value;
    SoundAssetTable* next;
};

static SoundAssetTable* Add(SoundAssetTable* table, StaticSoundAsset asset)
{
    // move to end.
    // add to end.
    SoundAssetTable* current = table;
    if (current->value.name == 0) {
        asset.stream = (struct StaticAudioStream*)malloc(sizeof(struct StaticAudioStream));
        current->value = asset;
        current->next = 0;
        return(table);
    }

    while (current->value.name != 0) {
        if (current->next == 0) {
            asset.stream = (struct StaticAudioStream*)malloc(sizeof(struct StaticAudioStream));

            current->next = (struct SoundAssetTable*)malloc(sizeof(struct SoundAssetTable));
            current->next->value = asset;
            current->next->next = 0;
            return(table);
        } 

        current = current->next;
    } 

    // return(table);
}

static StaticSoundAsset* Get(SoundAssetTable* table, char* name) 
{
    SoundAssetTable* current = table;
    while (current->value.name != 0) {
        if (strcmp(current->value.name, name) == 0) {
            return(&current->value);
        } 

        current = current->next;
    } 
    
    return(0);
}

