#include "audio.h"
#include "newgame.h"

static SoundAssetTable* Add(SoundAssetTable* table, AudioAsset asset, GameMemory* game_memory)
{
    // move to end.
    // add to end.
    SoundAssetTable* current = table;
    if (current->value.name[0] == 0) {
        // asset.name = (char*) malloc(32);
        current->value = asset;
        current->next = 0;
        return(table);
    }

    while (current->value.name != 0) {
        if (current->next == 0) {
            // asset.stream = (struct StaticAudioStream*)malloc(sizeof(struct StaticAudioStream));
            // asset.name = (char*) malloc(32);

            current->next = (struct SoundAssetTable*)malloc(sizeof(struct SoundAssetTable));
            game_memory->permanent_storage_remaining -= sizeof(struct SoundAssetTable);
            current->next->value = asset;
            current->next->next = 0;
            return(table);
        } 

        current = current->next;
    } 

    return(0);
}

static AudioAsset* Get(SoundAssetTable* table, char* name) 
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

