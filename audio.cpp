#include <windows.h>
#include <spatialaudioclient.h>
#include <audioclient.h>
#include <mmeapi.h>
#include <mmdeviceapi.h>
// #include <combaseapi.h> // For ComPtr.

#include "file_io.h"

#define REFTIMES_PER_SEC 10000000
#define REFTIMES_PER_MILLISEC 10000

#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

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

struct StaticAudioStream 
{
    IAudioClient* audio_client;
    IAudioRenderClient* render_client;
    REFERENCE_TIME buffer_duration;
    u32 buffer_frame_count; 
    BYTE* buffer;
    u8* media_ptr;
    i32 data_remaining;
};

struct SpatialAudioStream 
{
    ISpatialAudioClient* client;
    ISpatialAudioObjectRenderStream* render_stream;
    HANDLE buffer_completion_event;
};

static bool user_spatial_audio = true;
static bool all_sounds_stop = false;

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
    for (int counter = 0; counter < len; counter += 1) {
        (*enu).GetFormat(counter, &supported);
        int bleh = 4;        
    }

    SAFE_RELEASE(defaultDevice)
    SAFE_RELEASE(deviceEnum)

    return(spatial_audio_client);
}

// todo: each of these windows calls can return FAILED(hr), which should be taken care of somehow.
static IAudioClient* InitStaticAudioClient(WAVEFORMATEX format_24_48) 
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
    hr = audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, REFTIMES_PER_SEC, 0, &format_24_48, NULL);

    // might free audio_client, hopefully not!
    // SAFE_RELEASE(default_device)
    // SAFE_RELEASE(deviceEnum)

    return(audio_client);
}

static StaticAudioStream SetupStaticAudioStream(char* file_name, WAVEFORMATEX format_24_48, IAudioClient* audio_client) 
{
    HRESULT hr;
    StaticAudioStream result = {};

    // Load file into memory. 
    file_read_result media_data;
    PlatformReadEntireFile(file_name, &media_data);

    // Assuming this is a WAV file, we need to skip the first 44 bytes, which is the header of the file.
    result.media_ptr = (u8*)media_data.data + 44;
    result.data_remaining = media_data.data_len - 44;
    
    hr = audio_client->GetBufferSize(&result.buffer_frame_count);

    hr = audio_client->GetService(__uuidof(IAudioRenderClient), (void**)&result.render_client);

    // Grab the buffer for initial fill operation. The space for data has been allocated here.
    hr = result.render_client->GetBuffer(result.buffer_frame_count, &result.buffer);

    // Load buffer_frame_count amount of data from media_data to data shared buffer.
    // We are grabbing buffer_frame_count amount of 2 stereo channel * 24 bits... which is 6 bytes. 
    // u8* media_read_end = media_ptr + 6 * buffer_frame_count;
    memcpy(result.buffer, result.media_ptr, result.buffer_frame_count * 6);
    result.media_ptr += result.buffer_frame_count * 6;
    result.data_remaining -= result.buffer_frame_count * 6;
    
    // Release that buffer.
    hr = result.render_client->ReleaseBuffer(result.buffer_frame_count, 0);

    // Calculate how long to sleep for....
    result.buffer_duration = (double)REFTIMES_PER_SEC * result.buffer_frame_count / format_24_48.nSamplesPerSec;

    result.audio_client = audio_client;

    return(result);
}

    static u32 where_ptr_is = 48000 * 6;
    static u32 where_player_is = 0;
    static u32 where_ptr_should_be_next = 48000 * 6;

// note: for sounds that are playing.
static void PlayStaticAudio(std::vector<StaticAudioStream>* streams) 
{
    HRESULT hr;
    int counter = 0;
    std::vector<StaticAudioStream>::iterator it = std::begin((*streams));

    while (it != std::end((*streams))) {
        u32 padding_frames_count = 0;
        u32 used_frames = 0;
        
        // Sleep for half of buffer duration. This means approx half of buffer's audio is unplayed at this point.
        // todo: this will stall the ENTIRE thread, so we need to create different threads for each sound...
        Sleep((DWORD)((*it).buffer_duration / REFTIMES_PER_MILLISEC / 2));

        // How much buffer space is available now?
        hr = (*it).audio_client->GetCurrentPadding(&padding_frames_count);

        used_frames = (*it).buffer_frame_count - padding_frames_count;
        where_player_is += used_frames;

        // Get buffer..
        hr = (*it).render_client->GetBuffer(used_frames, &(*it).buffer);

        // study: might need to just use used_frames, as this might be == buffer_frame_count when we are out of
        //        data.
        (*it).data_remaining -= used_frames * 6;
        if ((*it).data_remaining <= 0) {
            hr = (*it).render_client->ReleaseBuffer(used_frames, 0);
            Sleep((DWORD)((*it).buffer_duration / REFTIMES_PER_MILLISEC / 2));

            // study: there may be a single audio client per stream, in which case, this would 
            //        stop all static audio files at once.
            hr = (*it).audio_client->Stop();
            
            SAFE_RELEASE((*streams).at(counter).audio_client)
            SAFE_RELEASE((*streams).at(counter).render_client)
            it = (*streams).erase(it);
            continue;
        }

        memcpy((*streams).at(counter).buffer, (*streams).at(counter).media_ptr, 
                used_frames * 6);

        (*it).media_ptr += used_frames * 6;

        hr = (*streams).at(counter).render_client->ReleaseBuffer(used_frames, 0);
        counter += 1;
        it += 1;
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
        float etc = it->position.x;
        float yada = it->position.x + 19;

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
