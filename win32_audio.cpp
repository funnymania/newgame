#ifndef AUDIO_CPP
#define AUDIO_CPP

#include <windows.h>
#include <spatialaudioclient.h>
#include <audioclient.h>
#include <mmeapi.h>
#include <mmdeviceapi.h>
// #include <combaseapi.h> // For ComPtr.

#include "audio.h"
#include "newgame.h"
#include "util.h"

#define REFTIMES_PER_SEC 10000000
#define REFTIMES_PER_MILLISEC 10000

#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

static StaticAudioStream SetupStaticAudioStream(AudioAsset* sound, GameMemory* game_memory);
static DWORD WINAPI PlayStaticAudio(LPVOID param);

// Container that the sound goes in to. So, from some audio source, we will
// be updating this container's frequency. 
struct AudioObj
{
    ISpatialAudioObject* audioObject;
    v3_float position;
    v3_float velocity;
    float volume;
    float frequency; // in Hz
    UINT totalFrameCount;
};

struct SpatialAudioStream 
{
    ISpatialAudioClient* client;
    ISpatialAudioObjectRenderStream* render_stream;
    HANDLE buffer_completion_event;
};


static bool user_spatial_audio = true;

// static SoundAssetTable sound_assets;

static IAudioClient* static_client_16_44;
static IAudioClient* static_client_24_48;
static IAudioClient* static_client_24_96;

static WAVEFORMATEX default_static_format = {
    WAVE_FORMAT_PCM,
    2, 
    48000L, 
    288000L, 
    6, // channels * bits / 8
    24, 
    0
};


// todo: Some kind of logging for if file reads crash.
static void LoadSound(char* file_name, SoundAssetTable** sound_assets, GameMemory* game_memory)
{
    AudioAsset new_sound = {};
    strcpy(new_sound.name, file_name);

    // Load file into memory. 
    file_read_result media_data;
    PlatformReadEntireFile(file_name, &media_data, game_memory);
    // game_memory->permanent_storage_remaining -= media_data.data_len;

    // todo: parse sample_rate, bits and channels from header data.
    // Assume, as far as audio is concerned, we only accept .WAV.
    ParseWavHeader((u8*)media_data.data, &new_sound);

    // todo: Skip bytes according to file type.
    new_sound.audio_data = (u8*)media_data.data;
    new_sound.audio_data_seek = new_sound.audio_data + 44;
    // new_sound.sample_rate = 48000;
    // new_sound.bits = 24;
    // new_sound.channels = 2;

    // Add to table.
    // SoundAssetTable* ptr = Add(&sound_assets, new_sound);

    *sound_assets = Add(*sound_assets, new_sound, game_memory);

    AudioAsset* aa = Get(*sound_assets, file_name);
    aa->audio_data_size = media_data.data_len;
    aa->data_remaining = media_data.data_len - 44;
}

static StaticAudioStream running_streams[32] = {0};

static int AddStream(StaticAudioStream stream) 
{
    for (int index = 0; index < 32; index += 1) {
        if (running_streams[index].audio_client == 0) {
            running_streams[index] = stream;
            return(index);
        }
    }

    // Max reached.
    return(-1);
}

static StaticAudioStream* Find(char* name) 
{
    for (int index = 0; index < 32; index += 1) {
        if (strcmp(running_streams[index].name, name)) {
            return &running_streams[index];
        }
    }

    return(0);
}

static SoundPlayResult StartPlaying(char* name, SoundAssetTable* sound_assets, GameMemory* game_memory)
{
    SoundPlayResult result = {};

    // note: this is malloc'd, so we can use the pointer.
    AudioAsset* to_play = Get(sound_assets, name);
    
    // Create the stream!
    StaticAudioStream stream = SetupStaticAudioStream(to_play, game_memory);

    // Hook up supported events required by game loop.
    HANDLE wait_events[4];
    for (int counter = 0; counter < 4; counter += 1) {
        wait_events[counter] = CreateEvent(
            0, 0, 0, 0
        );

        if (wait_events[counter] == NULL) 
        { 
            printf("CreateEvent error: %d\n", GetLastError() ); 
            ExitProcess(0); 
        } 
    }
    
    stream.wait_events[0] = wait_events[0];
    stream.wait_events[1] = wait_events[1];
    stream.wait_events[2] = wait_events[2];
    stream.wait_events[3] = wait_events[3];

    int index = AddStream(stream);
    
    if (index == -1) {
        result.response = 0; 
        return(result);
    }

    // Create/run the thread!
    HANDLE stream_thread = CreateThread(0, 0, PlayStaticAudio, &running_streams[index], 0, 0); // last arg is thread_id

    if (stream_thread == 0) {
        // error!
        OutputDebugString("Thread creation failed");
        result.response = 0;
    } else {
        result.response = 1;
    }

    return(result);
}

static SoundPlayResult StopPlaying(char* name, SoundAssetTable* sound_assets) 
{
    SoundPlayResult result = {};

    // AudioAsset* asset = Get(&sound_assets, name);

    // if (asset == 0) {
    //     result.response = 0;
    //     return(result);
    // }

    StaticAudioStream* stream = Find(name); 

    SetEvent(stream->wait_events[0]);
}

static SoundPlayResult Pause(char* name, SoundAssetTable* sound_assets) 
{
    SoundPlayResult result = {};
    result.response = 1;

    // AudioAsset* asset = Get(&sound_assets, name);

    // if (asset == 0) {
    //     result.response = 0;
    //     return(result);
    // }

    StaticAudioStream* stream = Find(name); 

    SetEvent(stream->wait_events[1]);
    return(result);
}

static SoundPlayResult Reverse(char* name, SoundAssetTable* sound_assets)
{
    SoundPlayResult result = {};
    result.response = 1;

    // StaticSoundAsset* asset = Get(&sound_assets, name);

    // if (asset == 0) {
    //     result.response = 0;
    //     return(result);
    // }

    StaticAudioStream* stream = Find(name); 

    SetEvent(stream->wait_events[3]);
    return(result);
}

static SoundPlayResult Unpause(char* name, SoundAssetTable* sound_assets) 
{
    SoundPlayResult result = {};
    result.response = 1;

    // StaticSoundAsset* asset = Get(&sound_assets, name);

    // if (asset == 0) {
    //     result.response = 0;
    //     return(result);
    // }

    StaticAudioStream* stream = Find(name); 

    SetEvent(stream->wait_events[2]);
    return(result);
}

void WriteToAudioObjectBuffer(FLOAT* buffer, UINT frameCount, FLOAT frequency, UINT samplingRate)
{
    const double PI = 4 * atan2(1.0, 1.0);
    static double _radPhase = 0.0;

    double step = 2 * PI * frequency / samplingRate;

    for (UINT i = 0; i < frameCount; i++)
    {
        double sample = sin(_radPhase);

        buffer[i] = FLOAT(sample);

        _radPhase += step; // next frame phase

        if (_radPhase >= 2 * PI)
        {
            _radPhase -= 2 * PI;
        }
    }
}

static ISpatialAudioClient* InitSpatialAudioClient() 
{
    HRESULT hr;
    IMMDeviceEnumerator* deviceEnum; // Must be Freed!
    IMMDevice* defaultDevice;   // Must be Freed!
    ISpatialAudioClient* spatial_audio_client; // Must be Freed!

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), 
            (void**)&deviceEnum);
    hr = deviceEnum->GetDefaultAudioEndpoint(EDataFlow::eRender, eMultimedia, &defaultDevice);

    // Activate ISpatialAudioClient on the desired audio-device 
    hr = defaultDevice->Activate(__uuidof(ISpatialAudioClient), CLSCTX_INPROC_SERVER, nullptr, 
            (void**)&spatial_audio_client);

    // hr = spatial_audio_client->IsAudioObjectFormatSupported(&format_24_48);

    // Get a list of supported audio formats.
    IAudioFormatEnumerator* enu;
    spatial_audio_client->GetSupportedAudioObjectFormatEnumerator(&enu);

    WAVEFORMATEX* supported;

    UINT32 len;
    hr = (*enu).GetCount(&len);
    for (u32 counter = 0; counter < len; counter += 1) {
        (*enu).GetFormat(counter, &supported);
    }

    SAFE_RELEASE(defaultDevice)
    SAFE_RELEASE(deviceEnum)

    return(spatial_audio_client);
}

static void AssignAudioClient(WAVEFORMATEX format, IAudioClient* audio_client)
{
    if (format.nSamplesPerSec == 44100) {
        static_client_16_44 = audio_client;
    } else if (format.nSamplesPerSec == 48000) {
        static_client_24_48 = audio_client;
    } else if (format.nSamplesPerSec == 96000) {
        static_client_24_96 = audio_client;
    } else {
        static_client_24_48 = audio_client;
    }
}

// todo: each of these windows calls can return FAILED(hr), which should be taken care of somehow.
static void InitStaticAudioClient(WAVEFORMATEX format) 
{
    HRESULT hr;

    IMMDeviceEnumerator* deviceEnum; // Must be Freed!
    IMMDevice* default_device;   // Must be Freed!
    IAudioClient* audio_client; // Must be Freed!

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), 
            (void**)&deviceEnum);
    hr = deviceEnum->GetDefaultAudioEndpoint(EDataFlow::eRender, eMultimedia, &default_device);

    // Activate IAudioClient.
    hr = default_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&audio_client);
   
    // Initialize everything.
    hr = audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, REFTIMES_PER_SEC, 0, &format, NULL);

    // perf: should be freed, hopefully does not free audio_client
    // SAFE_RELEASE(default_device)
    // SAFE_RELEASE(deviceEnum)

    AssignAudioClient(format, audio_client);
}

static StaticAudioStream SetupStaticAudioStream(AudioAsset* sound, GameMemory* game_memory) 
{
    HRESULT hr;
    IAudioClient* audio_client;
    StaticAudioStream stream = {};

    // Setup stream from AudioAsset.
    stream.media_ptr = sound->audio_data_seek;
    strcpy(stream.name, sound->name);
    stream.data_remaining = sound->data_remaining;
    stream.file_size = sound->audio_data_size;

    if (sound->sample_rate == 44100) {
        audio_client = static_client_16_44;
    } else if (sound->sample_rate == 48000) {
        audio_client = static_client_24_48;
    } else if (sound->sample_rate == 96000) {
        audio_client = static_client_24_96;
    } else {
        audio_client = static_client_24_48;
    }

    hr = audio_client->GetBufferSize(&stream.buffer_frame_count);

    hr = audio_client->GetService(__uuidof(IAudioRenderClient), (void**)&stream.render_client);

    // Grab the buffer for initial fill operation. The space for data has been allocated here.
    hr = stream.render_client->GetBuffer(stream.buffer_frame_count, &stream.buffer);
    game_memory->permanent_storage_remaining -= stream.buffer_frame_count * sound->bytes_per_frame;

    // Load buffer_frame_count amount of data from media_data to data shared buffer.
    // We are grabbing buffer_frame_count amount of 2 stereo channel * 24 bits... which is 6 bytes. 
    // u8* media_read_end = media_ptr + 6 * buffer_frame_count;
    memcpy(stream.buffer, stream.media_ptr, stream.buffer_frame_count * sound->bytes_per_frame);
    stream.media_ptr += stream.buffer_frame_count * sound->bytes_per_frame;
    stream.data_remaining -= stream.buffer_frame_count * sound->bytes_per_frame;
    
    // Release that buffer.
    hr = stream.render_client->ReleaseBuffer(stream.buffer_frame_count, 0);

    // Calculate how long to sleep for....
    stream.buffer_duration = (u64)REFTIMES_PER_SEC * stream.buffer_frame_count / sound->sample_rate;

    stream.audio_client = audio_client;
    stream.paused = false;

    return(stream);
}

// stud: if we had our own event system, all we would need from Windows for Audio Playing for Static Audio, is:
//          1. StartPlaying -> stream->audio_client->Start();   // for Start(), Unpause()
//          2. StopPlaying -> stream->audio_client->Stop();     // for Stop(), Pause()
//          3. GetBuffer ->                                     // To write audio data to.
//              hr = stream->audio_client->GetCurrentPadding(&padding_frames_count);
//              used_frames = stream->buffer_frame_count - padding_frames_count;
//              hr = stream->render_client->GetBuffer(used_frames, &stream->buffer);
//          4. ReleaseBuffer ->                                 // To Give that space back to OS.
//              hr = stream->render_client->ReleaseBuffer(used_frames, 0); 
//       For now, all of this is being handled in Windows, which is why all the audio playing code is here.
// note: for sounds that are playing.
static DWORD WINAPI PlayStaticAudio(LPVOID param) 
{
    HRESULT hr;
    StaticAudioStream* stream = (StaticAudioStream*) param;

    stream->audio_client->Start();

    bool play_in_reverse = false;
    bool play_in_reverse_first = true;

    while (true) {
        u32 padding_frames_count = 0;
        u32 used_frames = 0;
        u32 padding_track = 0;

		hr = stream->audio_client->GetCurrentPadding(&padding_track);
        
        // Sleep for half of buffer duration. This means approx half of buffer's audio is unplayed at this point.
        // Wakes upon Stop(), Pause(), etc.
        DWORD event_res = WaitForMultipleObjects(4, stream->wait_events, FALSE,
                (DWORD)(stream->buffer_duration / REFTIMES_PER_MILLISEC / 2));

        switch (event_res) {
            // Stop.
            case WAIT_OBJECT_0 + 0: {
                Sleep((DWORD)(stream->buffer_duration / REFTIMES_PER_MILLISEC / 2));
                stream->audio_client->Stop();
                SAFE_RELEASE(stream->audio_client)
                SAFE_RELEASE(stream->render_client)
                stream = 0;
                return(1);
            } break;

            // PAUSE
            case WAIT_OBJECT_0 + 1: {
                stream->audio_client->Stop();
                stream->paused = true;
            } break;

            case WAIT_OBJECT_0 + 2: {
                stream->audio_client->Start();
                stream->paused = false;
            } break;

            // play in reverse
            case WAIT_OBJECT_0 + 3: {
                //  sound->stream->audio_client->Start();
                //  sound->stream->paused = false;
                play_in_reverse = !play_in_reverse;
            } break;
        }

        if (stream->paused == false) {
            // How much buffer space is available now?
            hr = stream->audio_client->GetCurrentPadding(&padding_frames_count);

            if (hr == AUDCLNT_E_DEVICE_INVALIDATED) {
                // Device disconnect.
                hr = stream->audio_client->Stop();
                
                SAFE_RELEASE(stream->audio_client)
                SAFE_RELEASE(stream->render_client)
                stream = 0;

                // todo: Setup another audio client for a different/default device. 
                return(1);
            }

            used_frames = stream->buffer_frame_count - padding_frames_count;

            // Get buffer..
            // 0x88890006 means 'AUDCLNT_E_BUFFER_TOO_LARGE'
            // 0x88890007 means 'AUDCLNT_E_OUT_OF_ORDER' 
            hr = stream->render_client->GetBuffer(used_frames, &stream->buffer);

            // if (hr == AUDCLNT_E_BUFFER_TOO_LARGE) {
            //     int counter= 0;
            //     while (hr == AUDCLNT_E_BUFFER_TOO_LARGE) {
            //         hr = stream->audio_client->GetCurrentPadding(&padding_frames_count);
            //         used_frames = stream->buffer_frame_count - padding_frames_count;
            //         hr = stream->render_client->GetBuffer(used_frames, &stream->buffer);
            //         counter += 1;
            //     }

            //     if (hr != S_OK) {
            //         continue;
            //     }
            // }
            // else if (hr == AUDCLNT_E_BUFFER_ERROR) {
            //     hr = stream->audio_client->GetCurrentPadding(&padding_frames_count);
            //     used_frames = stream->buffer_frame_count - padding_frames_count;
            //     hr = stream->render_client->GetBuffer(used_frames, &stream->buffer);
            // }
            // else if (hr == AUDCLNT_E_BUFFER_SIZE_ERROR) {
            //     hr = stream->audio_client->GetCurrentPadding(&padding_frames_count);
            //     used_frames = stream->buffer_frame_count - padding_frames_count;
            //     hr = stream->render_client->GetBuffer(used_frames, &stream->buffer);
            // }
            // else if (hr == AUDCLNT_E_OUT_OF_ORDER) {
            //     continue;
            //     // hr = stream->audio_client->GetCurrentPadding(&padding_frames_count);
            //     // used_frames = stream->buffer_frame_count - padding_frames_count;
            //     // hr = stream->render_client->GetBuffer(used_frames, &stream->buffer);
            // }
            // else if (hr == AUDCLNT_E_DEVICE_INVALIDATED) {
            //     hr = stream->audio_client->GetCurrentPadding(&padding_frames_count);
            //     used_frames = stream->buffer_frame_count - padding_frames_count;
            //     hr = stream->render_client->GetBuffer(used_frames, &stream->buffer);
            // }
            // else if (hr == AUDCLNT_E_BUFFER_OPERATION_PENDING) {
            //     hr = stream->audio_client->GetCurrentPadding(&padding_frames_count);
            //     used_frames = stream->buffer_frame_count - padding_frames_count;
            //     hr = stream->render_client->GetBuffer(used_frames, &stream->buffer);
            // }
            // else if (hr == AUDCLNT_E_SERVICE_NOT_RUNNING) {
            //     hr = stream->audio_client->GetCurrentPadding(&padding_frames_count);
            //     used_frames = stream->buffer_frame_count - padding_frames_count;
            //     hr = stream->render_client->GetBuffer(used_frames, &stream->buffer);
            // }
            // else if (hr == E_POINTER) {
            //     hr = stream->audio_client->GetCurrentPadding(&padding_frames_count);
            //     used_frames = stream->buffer_frame_count - padding_frames_count;
            //     hr = stream->render_client->GetBuffer(used_frames, &stream->buffer);
            // }

            // study: might need to just use used_frames, as this might be == buffer_frame_count when we are out of
            //        data.
            if (play_in_reverse) {
                stream->data_remaining += used_frames * 6;
            } else {
                stream->data_remaining -= used_frames * 6;
            }

            if (stream->data_remaining <= 0 || stream->data_remaining >= stream->file_size) {
                hr = stream->render_client->ReleaseBuffer(used_frames, 0);
                Sleep((DWORD)(stream->buffer_duration / REFTIMES_PER_MILLISEC / 2));

                // study: there may be a single audio client per stream, in which case, this would 
                //        stop all static audio files at once.
                hr = stream->audio_client->Stop();
                
                SAFE_RELEASE(stream->audio_client)
                SAFE_RELEASE(stream->render_client)
                stream = 0;
                return(1);
            }

            if (play_in_reverse) {
                if (play_in_reverse_first == true) {
                    u8* reverse_ptr = stream->buffer + used_frames;
                    reverse_memcpy(reverse_ptr, stream->media_ptr, padding_frames_count * 6);
                    stream->media_ptr -= padding_frames_count * 6;

                    // u32 back_frames = used_frames - padding_frames_count;
                    reverse_memcpy(stream->buffer, stream->media_ptr, used_frames * 6);
                    play_in_reverse_first = false;
                } else {
                    reverse_memcpy(stream->buffer, stream->media_ptr, used_frames * 6);
                    stream->media_ptr -= used_frames * 6;
                }
            } else {
                memcpy(stream->buffer, stream->media_ptr, used_frames * 6);

                stream->media_ptr += used_frames * 6;
            }

            // todo: non-windowsy WAIT for some message.
            hr = stream->render_client->ReleaseBuffer(used_frames, 0);
        }
    }
}

// todo: Sine Wave (or even flat tones) are sounding like incomprehensible garbage.
static void PlaySpatialAudio(SpatialAudioStream* stream, WAVEFORMATEX format, std::vector<AudioObj>* spatial_sounds) 
{
    HRESULT hr;

    // Wait for a signal from the audio-engine to start the next processing pass
    if (WaitForSingleObject(stream->buffer_completion_event, 100) != WAIT_OBJECT_0)
    {
        return;
    }

    if ((*spatial_sounds).size() == 0) return;

    UINT32 availableDynamicObjectCount;
    UINT32 frameCount;

    // Begin the process of sending object data and metadata
    // Get the number of active objects that can be used to send object-data
    // Get the frame count that each buffer will be filled with 
    hr = stream->render_stream->BeginUpdatingAudioObjects(&availableDynamicObjectCount, &frameCount);

    BYTE* buffer;
    UINT32 bufferLength;

    // Loop through all dynamic audio objects
    std::vector<AudioObj>::iterator it = (*spatial_sounds).begin();

    while (it != (*spatial_sounds).end()) {
        it->audioObject->GetBuffer(&buffer, &bufferLength);

        if (it->totalFrameCount >= frameCount) {
            // Write audio data to the buffer
            WriteToAudioObjectBuffer(reinterpret_cast<float*>(buffer), frameCount, it->frequency, 
                    format.nSamplesPerSec);

            // Update the position and volume of the audio object
            it->position.x += it->velocity.x;
            it->position.y += it->velocity.y;
            it->position.z += it->velocity.z;
            it->audioObject->SetVolume(it->volume);

            it->totalFrameCount -= frameCount;

            ++it;
        } else {
            // If the audio object reaches its lifetime, set EndOfStream and release the object

            // todo: Write audio data to the buffer (see impl in Static above)
            WriteToAudioObjectBuffer(reinterpret_cast<float*>(buffer), it->totalFrameCount, it->frequency, 
                    format.nSamplesPerSec);

            // Set end of stream for the last buffer 
            hr = it->audioObject->SetEndOfStream(it->totalFrameCount);

            it->audioObject = nullptr; // Release the object

            it->totalFrameCount = 0;

            it = (*spatial_sounds).erase(it);
        }
    }

    // Let the audio-engine know that the object data are available for processing now
    hr = stream->render_stream->EndUpdatingAudioObjects();
}

// note: likely non-playing spatial audio when User switches off Spatial Sound while spatial audio playing.
static SpatialAudioStream SetupSpatialAudioStream(WAVEFORMATEX format_24_48, ISpatialAudioClient* client, 
        UINT32 max_spatial_obj) 
{
    HRESULT hr;
    SpatialAudioStream result = {};

    // Create the event that will be used to signal the client for more data.
    result.buffer_completion_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    // Set the maximum number of dynamic audio objects that will be used
    SpatialAudioObjectRenderStreamActivationParams streamParams;
    streamParams.ObjectFormat = &format_24_48;
    streamParams.StaticObjectTypeMask = AudioObjectType_None;
    streamParams.MinDynamicObjectCount = 0;
    streamParams.MaxDynamicObjectCount = min(max_spatial_obj, 4);
    streamParams.Category = AudioCategory_GameEffects;
    streamParams.EventHandle = result.buffer_completion_event;
    streamParams.NotifyObject = nullptr;

    PROPVARIANT pv;
    PropVariantInit(&pv);
    pv.vt = VT_BLOB;
    pv.blob.cbSize = sizeof(streamParams);
    pv.blob.pBlobData = (BYTE *)&streamParams;

    ISpatialAudioObjectRenderStream* spatialAudioStream;
    hr = client->ActivateSpatialAudioStream(&pv, __uuidof(spatialAudioStream), (void**)&spatialAudioStream);

    // todo: one or more args are invalid!
    if (hr == S_OK)
    {
        // cry;
    }

    // result.
    result.render_stream = spatialAudioStream;
    result.client = client;

    return(result);
}

// DEPRECATED.
// static void StaticAudioClientActivate(IMMDevice* default_device, WAVEFORMATEX* format_sixteen_44) 
// {
//     HRESULT hr;
//     IAudioClient *audio_client;
//     IAudioRenderClient *render_client;
//     DWORD flags = 0;
// 
//     // todo: each of these calls can return FAILED(hr), which should be taken care of somehow.
//     
//     // Activate IAudioClient.
//     hr = default_device->Activate(__uuidof(audio_client), CLSCTX_INPROC_SERVER, nullptr, (void**)&audio_client);
//    
//     // Can get a "default" format by querying here. We are just going to pass our own format for now.
//     // hr = audio_client->GetMixFormat(&format_sixteen_44);
// 
//     // Initialize everything.
//     REFERENCE_TIME requested_duration = REFTIMES_PER_SEC;
//     hr = audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, requested_duration, 0, format_sixteen_44, NULL);
// 
//     // Load file into memory. 
//     // todo: Loading scheme for streaming or preloading assets required!
//     file_read_result media_data;
//     PlatformReadEntireFile("Eerie_Town.wav", &media_data);
// 
//     // Assuming this is a WAV file, we need to skip the first 44 bytes, which is the header of the file.
//     u8* media_ptr = (u8*)media_data.data + 44;
//     i32 data_remaining = media_data.data_len - 44;
//     
//     u32 buffer_frame_count = 0;
//     hr = audio_client->GetBufferSize(&buffer_frame_count);
// 
//     hr = audio_client->GetService(__uuidof(render_client), (void**)&render_client);
// 
//     // Grab the buffer for initial fill operation. The space for data has been allocated here.
//     BYTE *data;
//     hr = render_client->GetBuffer(buffer_frame_count, &data);
// 
//     // Load buffer_frame_count amount of data from media_data to data shared buffer.
//     // We are grabbing buffer_frame_count amount of 2 stereo channel * 24 bits... which is 6 bytes. 
//     // u8* media_read_end = media_ptr + 6 * buffer_frame_count;
//     memcpy(data, media_ptr, 6 * buffer_frame_count);
//     data_remaining -= 6 * buffer_frame_count;
//     
//     // Release that buffer.
//     hr = render_client->ReleaseBuffer(buffer_frame_count, 0);
// 
//     // Calculate how long to sleep for....
//     REFERENCE_TIME actual_duration = (double)REFTIMES_PER_SEC * buffer_frame_count / format_sixteen_44->nSamplesPerSec;
// 
//     hr = audio_client->Start();
// 
//     u32 padding_frames_count = 0;
//     u32 available_frames = 0;
// 
//     // Set pointer to next bit of unplayed data.
//     // media_ptr = media_read_end;
// 
//     // This is the audio playing loop. If the flag has been set to AUDCLNT.... this means that we are out of data.
//     // todo: Support looping audio!
//     while (flags != AUDCLNT_BUFFERFLAGS_SILENT && all_sounds_stop != true) {
//         
//         // Sleep for half of buffer duration?
//         Sleep((DWORD)(actual_duration / REFTIMES_PER_MILLISEC / 2));
// 
//         // How much buffer space is available now?
//         hr = audio_client->GetCurrentPadding(&padding_frames_count);
// 
//         available_frames = buffer_frame_count - padding_frames_count;
// 
//         // Get buffer..
//         hr = render_client->GetBuffer(available_frames, &data);
// 
//         media_ptr += 6 * available_frames;
//         data_remaining -= 6 * available_frames;
// 
//         if (data_remaining <= 0) {
//             hr = render_client->ReleaseBuffer(available_frames, 0);
//             break;
//         }
// 
//         memcpy(data, media_ptr, 6 * buffer_frame_count);
//         hr = render_client->ReleaseBuffer(available_frames, 0);
//     }
// 
//     Sleep((DWORD)(actual_duration / REFTIMES_PER_MILLISEC / 2));
// 
//     hr = audio_client->Stop();
// 
//     // CoTaskMemFree(); // In case we got the struct for format from windows.
//     SAFE_RELEASE(audio_client)
//     SAFE_RELEASE(render_client)
// }

// DEPRECATED.
// static void AudioClientActivate() 
// {
//     // This will depend on the sample.
//     // Set up WAVEFORMATEX: 44.1khz, 16bit
//     // https://docs.microsoft.com/en-us/windows/win32/multimedia/using-the-waveformatex-structure
//     WAVEFORMATEX format_sixteen_44 = {};
//     format_sixteen_44.wFormatTag = WAVE_FORMAT_PCM; 
//     format_sixteen_44.nChannels = 2; 
//     format_sixteen_44.nSamplesPerSec = 48000L; 
//     format_sixteen_44.nAvgBytesPerSec = 288000L; 
//     format_sixteen_44.nBlockAlign = 6; // channels * bits / 8
//     format_sixteen_44.wBitsPerSample = 24; 
//     format_sixteen_44.cbSize = 0;
// 
//     HRESULT hr;
//     IMMDeviceEnumerator* deviceEnum; // Must be Freed!
//     IMMDevice* defaultDevice;   // Must be Freed!
// 
//     hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), 
//             (void**)&deviceEnum);
//     hr = deviceEnum->GetDefaultAudioEndpoint(EDataFlow::eRender, eMultimedia, &defaultDevice);
// 
//     // Activate ISpatialAudioClient on the desired audio-device 
//     ISpatialAudioClient* spatialAudioClient;
//     hr = defaultDevice->Activate(__uuidof(ISpatialAudioClient), CLSCTX_INPROC_SERVER, nullptr, 
//             (void**)&spatialAudioClient);
// 
//     hr = spatialAudioClient->IsAudioObjectFormatSupported(&format_sixteen_44);
// 
//     // Create the event that will be used to signal the client for more data.
//     HANDLE bufferCompletionEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
// 
//     UINT32 maxDynamicObjectCount;
//     hr = spatialAudioClient->GetMaxDynamicObjectCount(&maxDynamicObjectCount);
// 
//     // Dynamic objects are unsupported, fallback to static.
//     // note: this is entirely user decided via their Audio Settings. If they do not have Windows Sonic set for
//     //       spatialized sound, this will always be 0. We cannot override that through code.
//     if (maxDynamicObjectCount == 0)
//     {
//         StaticAudioClientActivate(defaultDevice, &format_sixteen_44);
//         SAFE_RELEASE(defaultDevice)
//         SAFE_RELEASE(deviceEnum)
//         return;
//     }
// 
// }

static void RenderAudio() 
{
    if (user_spatial_audio) {

    } else { 

    }
}

#endif
