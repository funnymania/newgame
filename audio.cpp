#include <windows.h>
#include <spatialaudioclient.h>
#include <audioclient.h>
#include <mmeapi.h>
#include <mmdeviceapi.h>
// #include <combaseapi.h>

#include "file_io.h"

#define REFTIMES_PER_SEC 10000000
#define REFTIMES_PER_MILLISEC 10000

#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

// Create an event callback.

// Get maxdynamicobjectcount.
// if zero, then just not supported, fallback to IAudioClient.

// Start an audio session.

// Create an audio stream from some file or just a square tone.
//


// Send an object in 3D space, which is emitting sound, to be rendered.

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

static void StaticAudioClientActivate(IMMDevice* default_device, WAVEFORMATEX* format_sixteen_44) 
{
    HRESULT hr;
    IAudioClient *audio_client;
    IAudioRenderClient *render_client;
    DWORD flags = 0;

    // todo: each of these calls can return FAILED(hr), which should be taken care of somehow.
    
    // Activate IAudioClient.
    hr = default_device->Activate(__uuidof(audio_client), CLSCTX_INPROC_SERVER, nullptr, (void**)&audio_client);
   
    // Can get a "default" format by querying here. We are just going to pass our own format for now.
    // hr = audio_client->GetMixFormat(&format_sixteen_44);

    // Initialize everything.
    REFERENCE_TIME requested_duration = REFTIMES_PER_SEC;
    hr = audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, requested_duration, 0, format_sixteen_44, NULL);

    // Load file into memory. 
    // todo: Loading scheme for streaming or preloading assets required!
    file_read_result media_data;
    PlatformReadEntireFile("Eerie_Town.wav", &media_data);

    // Assuming this is a WAV file, we need to skip the first 44 bytes, which is the header of the file.
    u8* media_ptr = (u8*)media_data.data + 44;
    i32 data_remaining = media_data.data_len - 44;
    
    u32 buffer_frame_count = 0;
    hr = audio_client->GetBufferSize(&buffer_frame_count);

    hr = audio_client->GetService(__uuidof(render_client), (void**)&render_client);

    // Grab the buffer for initial fill operation. The space for data has been allocated here.
    BYTE *data;
    hr = render_client->GetBuffer(buffer_frame_count, &data);

    // Load buffer_frame_count amount of data from media_data to data shared buffer.
    // We are grabbing buffer_frame_count amount of 2 stereo channel * 24 bits... which is 6 bytes. 
    // u8* media_read_end = media_ptr + 6 * buffer_frame_count;
    memcpy(data, media_ptr, 6 * buffer_frame_count);
    data_remaining -= 6 * buffer_frame_count;
    
    // Release that buffer.
    hr = render_client->ReleaseBuffer(buffer_frame_count, 0);

    // Calculate how long to sleep for....
    REFERENCE_TIME actual_duration = (double)REFTIMES_PER_SEC * buffer_frame_count / format_sixteen_44->nSamplesPerSec;

    hr = audio_client->Start();

    u32 padding_frames_count = 0;
    u32 available_frames = 0;

    // Set pointer to next bit of unplayed data.
    // media_ptr = media_read_end;

    // This is the audio playing loop. If the flag has been set to AUDCLNT.... this means that we are out of data.
    // todo: Support looping audio!
    while (flags != AUDCLNT_BUFFERFLAGS_SILENT && all_sounds_stop != true) {
        
        // Sleep for half of buffer duration?
        Sleep((DWORD)(actual_duration / REFTIMES_PER_MILLISEC / 2));

        // How much buffer space is available now?
        hr = audio_client->GetCurrentPadding(&padding_frames_count);

        available_frames = buffer_frame_count - padding_frames_count;

        // Get buffer..
        hr = render_client->GetBuffer(available_frames, &data);

        media_ptr += 6 * available_frames;
        data_remaining -= 6 * available_frames;

        if (data_remaining <= 0) {
            hr = render_client->ReleaseBuffer(available_frames, 0);
            break;
        }

        memcpy(data, media_ptr, 6 * buffer_frame_count);
        hr = render_client->ReleaseBuffer(available_frames, 0);
    }

    Sleep((DWORD)(actual_duration / REFTIMES_PER_MILLISEC / 2));

    hr = audio_client->Stop();

    // CoTaskMemFree(); // In case we got the struct for format from windows.
    SAFE_RELEASE(audio_client)
    SAFE_RELEASE(render_client)
}

static void AudioClientActivate() 
{
    // This will depend on the sample.
    // Set up WAVEFORMATEX: 44.1khz, 16bit
    // https://docs.microsoft.com/en-us/windows/win32/multimedia/using-the-waveformatex-structure
    WAVEFORMATEX format_sixteen_44 = {};
    format_sixteen_44.wFormatTag = WAVE_FORMAT_PCM; 
    format_sixteen_44.nChannels = 2; 
    format_sixteen_44.nSamplesPerSec = 48000L; 
    format_sixteen_44.nAvgBytesPerSec = 288000L; 
    format_sixteen_44.nBlockAlign = 6; // channels * bits / 8
    format_sixteen_44.wBitsPerSample = 24; 
    format_sixteen_44.cbSize = 0;

    HRESULT hr;
    IMMDeviceEnumerator* deviceEnum; // Must be Freed!
    IMMDevice* defaultDevice;   // Must be Freed!

    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), 
            (void**)&deviceEnum);
    hr = deviceEnum->GetDefaultAudioEndpoint(EDataFlow::eRender, eMultimedia, &defaultDevice);

    // Activate ISpatialAudioClient on the desired audio-device 
    ISpatialAudioClient* spatialAudioClient;
    hr = defaultDevice->Activate(__uuidof(ISpatialAudioClient), CLSCTX_INPROC_SERVER, nullptr, 
            (void**)&spatialAudioClient);


    hr = spatialAudioClient->IsAudioObjectFormatSupported(&format_sixteen_44);

    // Create the event that will be used to signal the client for more data.
    HANDLE bufferCompletionEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    UINT32 maxDynamicObjectCount;
    hr = spatialAudioClient->GetMaxDynamicObjectCount(&maxDynamicObjectCount);

    // Dynamic objects are unsupported, fallback to static.
    // note: this is entirely user decided via their Audio Settings. If they do not have Windows Sonic set for
    //       spatialized sound, this will always be 0. We cannot override that through code.
    if (maxDynamicObjectCount == 0)
    {
        StaticAudioClientActivate(defaultDevice, &format_sixteen_44);
        SAFE_RELEASE(defaultDevice)
        SAFE_RELEASE(deviceEnum)
        return;
    }

    // Set the maximum number of dynamic audio objects that will be used
    SpatialAudioObjectRenderStreamActivationParams streamParams;
    streamParams.ObjectFormat = &format_sixteen_44;
    streamParams.StaticObjectTypeMask = AudioObjectType_None;
    streamParams.MinDynamicObjectCount = 0;
    streamParams.MaxDynamicObjectCount = min(maxDynamicObjectCount, 4);
    streamParams.Category = AudioCategory_GameEffects;
    streamParams.EventHandle = bufferCompletionEvent;
    streamParams.NotifyObject = nullptr;

    PROPVARIANT pv;
    PropVariantInit(&pv);
    pv.vt = VT_BLOB;
    pv.blob.cbSize = sizeof(streamParams);
    pv.blob.pBlobData = (BYTE *)&streamParams;

    ISpatialAudioObjectRenderStream* spatialAudioStream;;
    hr = spatialAudioClient->ActivateSpatialAudioStream(&pv, __uuidof(spatialAudioStream), (void**)&spatialAudioStream);

    // Start streaming / rendering 
    hr = spatialAudioStream->Start();

    // Activate a new dynamic audio object
    std::vector<AudioObj> objectVector;

    ISpatialAudioObject* audioObject;
    hr = spatialAudioStream->ActivateSpatialAudioObject(AudioObjectType::AudioObjectType_Dynamic, &audioObject);

    // If SPTLAUDCLNT_E_NO_MORE_OBJECTS is returned, there are no more available objects
    if (SUCCEEDED(hr))
    {
        // Init new struct with the new audio object.
        AudioObj obj = {
            audioObject,
            { -100, 0, 0 },
            { 0.001, 0, 0 },
            1,
            3000,
            format_sixteen_44.nSamplesPerSec * 5 // 5 seconds of audio samples
        };

        objectVector.insert(objectVector.begin(), obj);
    }

    do
    {
        // Wait for a signal from the audio-engine to start the next processing pass
        if (WaitForSingleObject(bufferCompletionEvent, 100) != WAIT_OBJECT_0)
        {
            break;
        }

        UINT32 availableDynamicObjectCount;
        UINT32 frameCount;

        // Begin the process of sending object data and metadata
        // Get the number of active objects that can be used to send object-data
        // Get the frame count that each buffer will be filled with 
        hr = spatialAudioStream->BeginUpdatingAudioObjects(&availableDynamicObjectCount, &frameCount);

        BYTE* buffer;
        UINT32 bufferLength;

        // Loop through all dynamic audio objects
        std::vector<AudioObj>::iterator it = objectVector.begin();
        while (it != objectVector.end())
        {
            it->audioObject->GetBuffer(&buffer, &bufferLength);

            if (it->totalFrameCount >= frameCount)
            {
                // Write audio data to the buffer
                WriteToAudioObjectBuffer(reinterpret_cast<float*>(buffer), frameCount, it->frequency, format_sixteen_44.nSamplesPerSec);

                // Update the position and volume of the audio object
                it->audioObject->SetPosition(it->position.x, it->position.y, it->position.z);
                it->position.x += it->velocity.x;
                it->position.y += it->velocity.y;
                it->position.z += it->velocity.z;
                it->audioObject->SetVolume(it->volume);

                it->totalFrameCount -= frameCount;

                ++it;
            }
            else
            {
                // If the audio object reaches its lifetime, set EndOfStream and release the object

                // todo: Write audio data to the buffer (see impl in Static above)
                WriteToAudioObjectBuffer(reinterpret_cast<float*>(buffer), it->totalFrameCount, it->frequency, format_sixteen_44.nSamplesPerSec);

                // Set end of stream for the last buffer 
                hr = it->audioObject->SetEndOfStream(it->totalFrameCount);

                it->audioObject = nullptr; // Release the object

                it->totalFrameCount = 0;

                it = objectVector.erase(it);
            }
        }

        // Let the audio-engine know that the object data are available for processing now
        hr = spatialAudioStream->EndUpdatingAudioObjects();
    } while (objectVector.size() > 0);

    // Stop the stream 
    hr = spatialAudioStream->Stop();

    // We don't want to start again, so reset the stream to free it's resources.
    hr = spatialAudioStream->Reset();

    CloseHandle(bufferCompletionEvent);
}

static void RenderAudio() 
{
    if (user_spatial_audio) {

    } else { 

    }
}
