/*
    ReactOS
    Kernel Streaming API

    by Andrew Greenwood

    NOTES:
    This is a basic stubbing of the Kernel Streaming API header. It is
    very incomplete - a lot of the #defines are not set to any value at all.

    Some of the structs/funcs may be incorrectly grouped.

    GUIDs need to be defined properly.

    AVStream functionality (XP and above, DirectX 8.0 and above) will NOT
    implemented for a while.

    Some example code for interaction from usermode:
    DeviceIoControl(
        FilterHandle,
        IOCTL_KS_PROPERTY,
        &Property,
        sizeof(KSPROPERTY),
        &SeekingCapabilities,
        sizeof(KS_SEEKING_CAPABILITIES),
        &BytesReturned,
        &Overlapped);
*/

#ifndef KS_H
#define KS_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BUILDING_KS
    #define KSDDKAPI
#else
    #define KSDDKAPI DECLSPEC_IMPORT
#endif

/* TODO */
#define KSDDKAPI


typedef PVOID PKSWORKER;

/* ===============================================================
    I/O Control Codes
*/

#define IOCTL_KS_DISABLE_EVENT \
    CTL_CODE( \
        FILE_DEVICE_KS, \
        0x000, \
        METHOD_NEITHER, \
        FILE_ANY_ACCESS)

#define IOCTL_KS_ENABLE_EVENT \
    CTL_CODE( \
        FILE_DEVICE_KS, \
        0x001, \
        METHOD_NEITHER, \
        FILE_ANY_ACCESS)

#define IOCTL_KS_METHOD \
    CTL_CODE( \
        FILE_DEVICE_KS, \
        0x002, \
        METHOD_NEITHER, \
        FILE_ANY_ACCESS)

#define IOCTL_KS_PROPERTY \
    CTL_CODE( \
        FILE_DEVICE_KS, \
        0x003, \
        METHOD_NEITHER, \
        FILE_ANY_ACCESS)

#define IOCTL_KS_WRITE_STREAM \
    CTL_CODE( \
        FILE_DEVICE_KS, \
        0x004, \
        METHOD_NEITHER, \
        FILE_WRITE_ACCESS)

#define IOCTL_KS_READ_STREAM \
    CTL_CODE( \
        FILE_DEVICE_KS, \
        0x005, \
        METHOD_NEITHER, \
        FILE_READ_ACCESS)

#define IOCTL_KS_RESET_STATE \
    CTL_CODE( \
        FILE_DEVICE_KS, \
        0x006, \
        METHOD_NEITHER, \
        FILE_ANY_ACCESS)


/* ===============================================================
    Clock Properties/Methods/Events
*/

#define KSPROPSETID_Clock \
    0xDF12A4C0L, 0xAC17, 0x11CF, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00

typedef enum
{
    KSPROPERTY_CLOCK_TIME,
    KSPROPERTY_CLOCK_PHYSICALTIME,
    KSPROPERTY_CORRELATEDTIME,
    KSPROPERTY_CORRELATEDPHYSICALTIME,
    KSPROPERTY_CLOCK_RESOLUTION,
    KSPROPERTY_CLOCK_STATE,
    KSPROPERTY_CLOCK_FUNCTIONTABLE
} KSPROPERTY_CLOCK;

#define KSEVENTSETID_Clock \
    0x364D8E20L, 0x62C7, 0x11CF, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00

typedef enum
{
    KSEVENT_CLOCK_INTERVAL_MARK,
    KSEVENT_CLOCK_POSITION_MARK
} KSEVENT_CLOCK_POSITION;


/* ===============================================================
    Connection Properties/Methods/Events
*/

#define KSPROPSETID_Connection \
    0x1D58C920L, 0xAC9B, 0x11CF, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00

typedef enum
{
    KSPROPERTY_CONNECTION_STATE,
    KSPROPERTY_CONNECTION_PRIORITY,
    KSPROPERTY_CONNECTION_DATAFORMAT,
    KSPROPERTY_CONNECTION_ALLOCATORFRAMING,
    KSPROPERTY_CONNECTION_PROPOSEDATAFORMAT,
    KSPROPERTY_CONNECTION_ACQUIREORDERING,
    KSPROPERTY_CONNECTION_ALLOCATORFRAMING_EX,
    KSPROPERTY_CONNECTION_STARTAT
} KSPROPERTY_CONNECTION;

#define KSEVENTSETID_Connection \
    0x7f4bcbe0L, 0x9ea5, 0x11cf, 0xa5, 0xd6, 0x28, 0xdb, 0x04, 0xc1, 0x00, 0x00

typedef enum
{
    KSEVENT_CONNECTION_POSITIONUPDATE,
    KSEVENT_CONNECTION_DATADISCONTINUITY,
    KSEVENT_CONNECTION_TIMEDISCONTINUITY,
    KSEVENT_CONNECTION_PRIORITY,
    KSEVENT_CONNECTION_ENDOFSTREAM
} KSEVENT_CONNECTION;


/* ===============================================================
    General
    Properties/Methods/Events
*/

#define KSPROPSETID_General \
    0x1464EDA5L, 0x6A8F, 0x11D1, 0x9A, 0xA7, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96

typedef enum
{
    KSPROPERTY_GENERAL_COMPONENTID
} KSPROPERTY_GENERAL;


/* ===============================================================
    Graph Manager
    Properties/Methods/Events
*/

#define KSPROPSETID_GM \
    0xAF627536L, 0xE719, 0x11D2, 0x8A, 0x1D, 0x00, 0x60, 0x97, 0xD2, 0xDF, 0x5D

typedef enum
{
    KSPROPERTY_GM_GRAPHMANAGER,
    KSPROPERTY_GM_TIMESTAMP_CLOCK,
    KSPROPERTY_GM_RATEMATCH,
    KSPROPERTY_GM_RENDERCLOCK
} KSPROPERTY_GM;


/* ===============================================================
    Media Seeking
    Properties/Methods/Events
*/

#define KSPROPSETID_MediaSeeking \
    0xEE904F0CL, 0xD09B, 0x11D0, 0xAB, 0xE9, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96

typedef enum
{
    KSPROPERTY_MEDIASEEKING_CAPABILITIES,
    KSPROPERTY_MEDIASEEKING_FORMATS,
    KSPROPERTY_MEDIASEEKING_TIMEFORMAT,
    KSPROPERTY_MEDIASEEKING_POSITION,
    KSPROPERTY_MEDIASEEKING_STOPPOSITION,
    KSPROPERTY_MEDIASEEKING_POSITIONS,
    KSPROPERTY_MEDIASEEKING_DURATION,
    KSPROPERTY_MEDIASEEKING_AVAILABLE,
    KSPROPERTY_MEDIASEEKING_PREROLL,
    KSPROPERTY_MEDIASEEKING_CONVERTTIMEFORMAT
} KSPROPERTY_MEDIASEEKING;


/* ===============================================================
    Pin
    Properties/Methods/Events
*/

#define KSPROPSETID_Pin \
    0x8C134960L, 0x51AD, 0x11CF, 0x87, 0x8A, 0x94, 0xF8, 0x01, 0xC1, 0x00, 0x00

typedef enum
{
    KSPROPERTY_PIN_CINSTANCES,
    KSPROPERTY_PIN_CTYPES,
    KSPROPERTY_PIN_DATAFLOW,
    KSPROPERTY_PIN_DATARANGES,
    KSPROPERTY_PIN_DATAINTERSECTION,
    KSPROPERTY_PIN_INTERFACES,
    KSPROPERTY_PIN_MEDIUMS,
    KSPROPERTY_PIN_COMMUNICATION,
    KSPROPERTY_PIN_GLOBALCINSTANCES,
    KSPROPERTY_PIN_NECESSARYINSTANCES,
    KSPROPERTY_PIN_PHYSICALCONNECTION,
    KSPROPERTY_PIN_CATEGORY,
    KSPROPERTY_PIN_NAME,
    KSPROPERTY_PIN_CONSTRAINEDDATARANGES,
    KSPROPERTY_PIN_PROPOSEDATAFORMAT
} KSPROPERTY_PIN;


/* ===============================================================
    Quality
    Properties/Methods/Events
*/

#define KSPROPSETID_Quality \
    0xD16AD380L, 0xAC1A, 0x11CF, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00

typedef enum
{
    KSPROPERTY_QUALITY_REPORT,
    KSPROPERTY_QUALITY_ERROR
} KSPROPERTY_QUALITY;


/* ===============================================================
    Stream
    Properties/Methods/Events
*/

#define KSPROPSETID_Stream \
    0x65aaba60L, 0x98ae, 0x11cf, 0xa1, 0x0d, 0x00, 0x20, 0xaf, 0xd1, 0x56, 0xe4

typedef enum
{
    KSPROPERTY_STREAM_ALLOCATOR,
    KSPROPERTY_STREAM_QUALITY,
    KSPROPERTY_STREAM_DEGRADATION,
    KSPROPERTY_STREAM_MASTERCLOCK,
    KSPROPERTY_STREAM_TIMEFORMAT,
    KSPROPERTY_STREAM_PRESENTATIONTIME,
    KSPROPERTY_STREAM_PRESENTATIONEXTENT,
    KSPROPERTY_STREAM_FRAMETIME,
    KSPROPERTY_STREAM_RATECAPABILITY,
    KSPROPERTY_STREAM_RATE,
    KSPROPERTY_STREAM_PIPE_ID
} KSPROPERTY_STREAM;


/* ===============================================================
    StreamAllocator
    Properties/Methods/Events
*/

#define KSPROPSETID_StreamAllocator \
    0xcf6e4342L, 0xec87, 0x11cf, 0xa1, 0x30, 0x00, 0x20, 0xaf, 0xd1, 0x56, 0xe4

typedef enum
{
    KSPROPERTY_STREAMALLOCATOR_FUNCTIONTABLE,
    KSPROPERTY_STREAMALLOCATOR_STATUS
} KSPROPERTY_STREAMALLOCATOR;

#define KSMETHODSETID_StreamAllocator \
    0xcf6e4341L, 0xec87, 0x11cf, 0xa1, 0x30, 0x00, 0x20, 0xaf, 0xd1, 0x56, 0xe4

typedef enum
{
    KSMETHOD_STREAMALLOCATOR_ALLOC,
    KSMETHOD_STREAMALLOCATOR_FREE
} KSMETHOD_STREAMALLOCATOR;


#define KSEVENTSETID_StreamAllocator

typedef enum
{
    KSEVENT_STREAMALLOCATOR_INTERNAL_FREEFRAME,
    KSEVENT_STREAMALLOCATOR_FREEFRAME
} KSEVENT_STREAMALLOCATOR;


/* ===============================================================
    StreamInterface
    Properties/Methods/Events
*/

#define KSPROPSETID_StreamInterface \
    0x1fdd8ee1L, 0x9cd3, 0x11d0, 0x82, 0xaa, 0x00, 0x00, 0xf8, 0x22, 0xfe, 0x8a

typedef enum
{
    KSPROPERTY_STREAMINTERFACE_HEADERSIZE
} KSPROPERTY_STREAMINTERFACE;


/* ===============================================================
    Topology
    Properties/Methods/Events
*/

#define KSPROPSETID_Topology \
    0x720D4AC0L, 0x7533, 0x11D0, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00

typedef enum
{
    KSPROPERTY_TOPOLOGY_CATEGORIES,
    KSPROPERTY_TOPOLOGY_CONNECTIONS,
    KSPROPERTY_TOPOLOGY_NAME,
    KSPROPERTY_TOPOLOGY_NODES
} KSPROPERTY_TOPOLOGY;



/* ===============================================================
    Property Sets for audio drivers - TODO
*/

#define KSPROPSETID_AC3
/*
    KSPROPERTY_AC3_ALTERNATE_AUDIO 
    KSPROPERTY_AC3_BIT_STREAM_MODE 
    KSPROPERTY_AC3_DIALOGUE_LEVEL 
    KSPROPERTY_AC3_DOWNMIX 
    KSPROPERTY_AC3_ERROR_CONCEALMENT 
    KSPROPERTY_AC3_LANGUAGE_CODE 
    KSPROPERTY_AC3_ROOM_TYPE
*/

#define KSPROPSETID_Acoustic_Echo_Cancel
/*
    KSPROPERTY_AEC_MODE 
    KSPROPERTY_AEC_NOISE_FILL_ENABLE 
    KSPROPERTY_AEC_STATUS
*/

#define KSPROPSETID_Audio
/*
    KSPROPERTY_AUDIO_3D_INTERFACE
    KSPROPERTY_AUDIO_AGC 
    KSPROPERTY_AUDIO_ALGORITHM_INSTANCE 
    KSPROPERTY_AUDIO_BASS 
    KSPROPERTY_AUDIO_BASS_BOOST 
    KSPROPERTY_AUDIO_CHANNEL_CONFIG 
    KSPROPERTY_AUDIO_CHORUS_LEVEL 
    KSPROPERTY_AUDIO_COPY_PROTECTION 
    KSPROPERTY_AUDIO_CPU_RESOURCES 
    KSPROPERTY_AUDIO_DELAY 
    KSPROPERTY_AUDIO_DEMUX_DEST 
    KSPROPERTY_AUDIO_DEV_SPECIFIC 
    KSPROPERTY_AUDIO_DYNAMIC_RANGE 
    KSPROPERTY_AUDIO_DYNAMIC_SAMPLING_RATE 
    KSPROPERTY_AUDIO_EQ_BANDS 
    KSPROPERTY_AUDIO_EQ_LEVEL 
    KSPROPERTY_AUDIO_FILTER_STATE 
    KSPROPERTY_AUDIO_LATENCY 
    KSPROPERTY_AUDIO_LOUDNESS 
    KSPROPERTY_AUDIO_MANUFACTURE_GUID 
    KSPROPERTY_AUDIO_MID 
    KSPROPERTY_AUDIO_MIX_LEVEL_CAPS 
    KSPROPERTY_AUDIO_MIX_LEVEL_TABLE 
    KSPROPERTY_AUDIO_MUTE 
    KSPROPERTY_AUDIO_MUX_SOURCE 
    KSPROPERTY_AUDIO_NUM_EQ_BANDS 
    KSPROPERTY_AUDIO_PEAKMETER
    KSPROPERTY_AUDIO_POSITION 
    KSPROPERTY_AUDIO_PREFERRED_STATUS 
    KSPROPERTY_AUDIO_PRODUCT_GUID 
    KSPROPERTY_AUDIO_QUALITY 
    KSPROPERTY_AUDIO_REVERB_LEVEL 
    KSPROPERTY_AUDIO_SAMPLING_RATE 
    KSPROPERTY_AUDIO_STEREO_ENHANCE 
    KSPROPERTY_AUDIO_STEREO_SPEAKER_GEOMETRY 
    KSPROPERTY_AUDIO_SURROUND_ENCODE 
    KSPROPERTY_AUDIO_TREBLE 
    KSPROPERTY_AUDIO_VOLUMELEVEL 
    KSPROPERTY_AUDIO_WIDE_MODE 
    KSPROPERTY_AUDIO_WIDENESS
*/

#define KSPROPSETID_AudioGfx
/*
    KSPROPERTY_AUDIOGFX_CAPTURETARGETDEVICEID
    KSPROPERTY_AUDIOGFX_RENDERTARGETDEVICEID
*/

#define KSPROPSETID_DirectSound3DBuffer
/*
    KSPROPERTY_DIRECTSOUND3DBUFFER_ALL 
    KSPROPERTY_DIRECTSOUND3DBUFFER_CONEANGLES 
    KSPROPERTY_DIRECTSOUND3DBUFFER_CONEORIENTATION 
    KSPROPERTY_DIRECTSOUND3DBUFFER_CONEOUTSIDEVOLUME 
    KSPROPERTY_DIRECTSOUND3DBUFFER_MAXDISTANCE 
    KSPROPERTY_DIRECTSOUND3DBUFFER_MINDISTANCE 
    KSPROPERTY_DIRECTSOUND3DBUFFER_MODE 
    KSPROPERTY_DIRECTSOUND3DBUFFER_POSITION 
    KSPROPERTY_DIRECTSOUND3DBUFFER_VELOCITY
*/

#define KSPROPSETID_DirectSound3DListener
/*
    KSPROPERTY_DIRECTSOUND3DLISTENER_ALL 
    KSPROPERTY_DIRECTSOUND3DLISTENER_ALLOCATION
    KSPROPERTY_DIRECTSOUND3DLISTENER_BATCH 
    KSPROPERTY_DIRECTSOUND3DLISTENER_DISTANCEFACTOR 
    KSPROPERTY_DIRECTSOUND3DLISTENER_DOPPLERFACTOR 
    KSPROPERTY_DIRECTSOUND3DLISTENER_ORIENTATION 
    KSPROPERTY_DIRECTSOUND3DLISTENER_POSITION 
    KSPROPERTY_DIRECTSOUND3DLISTENER_ROLLOFFFACTOR 
    KSPROPERTY_DIRECTSOUND3DLISTENER_VELOCITY
*/

#define KSPROPSETID_DrmAudioStream
/*
    KSPROPERTY_DRMAUDIOSTREAM_CONTENTID
*/

#define KSPROPSETID_Hrtf3d
/*
    KSPROPERTY_HRTF3D_FILTER_FORMAT 
    KSPROPERTY_HRTF3D_INITIALIZE 
    KSPROPERTY_HRTF3D_PARAMS
*/

#define KSPROPSETID_Itd3d
/*
    KSPROPERTY_ITD3D_PARAMS
*/

#define KSPROPSETID_Synth
/*
    KSPROPERTY_SYNTH_CAPS 
    KSPROPERTY_SYNTH_CHANNELGROUPS 
    KSPROPERTY_SYNTH_LATENCYCLOCK 
    KSPROPERTY_SYNTH_MASTERCLOCK 
    KSPROPERTY_SYNTH_PORTPARAMETERS 
    KSPROPERTY_SYNTH_RUNNINGSTATS 
    KSPROPERTY_SYNTH_VOICEPRIORITY 
    KSPROPERTY_SYNTH_VOLUME 
    KSPROPERTY_SYNTH_VOLUMEBOOST
*/

#define KSPROPSETID_Synth_Dls
/*
    KSPROPERTY_SYNTH_DLS_APPEND 
    KSPROPERTY_SYNTH_DLS_COMPACT 
    KSPROPERTY_SYNTH_DLS_DOWNLOAD 
    KSPROPERTY_SYNTH_DLS_UNLOAD 
    KSPROPERTY_SYNTH_DLS_WAVEFORMAT
*/

#define KSPROPSETID_Sysaudio
/*
    KSPROPERTY_SYSAUDIO_COMPONENT_ID 
    KSPROPERTY_SYSAUDIO_CREATE_VIRTUAL_SOURCE 
    KSPROPERTY_SYSAUDIO_DEVICE_COUNT 
    KSPROPERTY_SYSAUDIO_DEVICE_FRIENDLY_NAME 
    KSPROPERTY_SYSAUDIO_DEVICE_INSTANCE 
    KSPROPERTY_SYSAUDIO_DEVICE_INTERFACE_NAME 
    KSPROPERTY_SYSAUDIO_INSTANCE_INFO 
    KSPROPERTY_SYSAUDIO_SELECT_GRAPH
*/

#define KSPROPSETID_Sysaudio_Pin
/*
    KSPROPERTY_SYSAUDIO_ATTACH_VIRTUAL_SOURCE
*/

#define KSPROPSETID_TopologyNode
/*
    KSPROPERTY_TOPOLOGYNODE_ENABLE 
    KSPROPERTY_TOPOLOGYNODE_RESET
*/


/* ===============================================================
    Interface Sets - TODO
*/

#define KSINTERFACESETID_Media

#define KSINTERFACESETID_Standard
#define KSINTERFACE_STANDARD_STREAMING
#define KSINTERFACE_STANDARD_LOOPED_STREAMING
#define KSINTERFACE_STANDARD_CONTROL


/* ===============================================================
    Event Sets for audio drivers - TODO
*/
#define KSEVENTSETID_AudioControlChange
/*
    KSEVENT_CONTROL_CHANGE
*/



/* ===============================================================
    Node Types
*/
/*
    KSNODETYPE_3D_EFFECTS 
    KSNODETYPE_ACOUSTIC_ECHO_CANCEL
    KSNODETYPE_ADC 
    KSNODETYPE_AGC 
    KSNODETYPE_CHORUS 
    KSNODETYPE_DAC 
    KSNODETYPE_DELAY 
    KSNODETYPE_DEMUX 
    KSNODETYPE_DEV_SPECIFIC 
    KSNODETYPE_DMSYNTH 
    KSNODETYPE_DMSYNTH_CAPS 
    KSNODETYPE_DRM_DESCRAMBLE 
    KSNODETYPE_EQUALIZER 
    KSNODETYPE_LOUDNESS 
    KSNODETYPE_MUTE 
    KSNODETYPE_MUX 
    KSNODETYPE_PEAKMETER
    KSNODETYPE_PROLOGIC_DECODER 
    KSNODETYPE_PROLOGIC_ENCODER 
    KSNODETYPE_REVERB 
    KSNODETYPE_SRC 
    KSNODETYPE_STEREO_ENHANCE
    KSNODETYPE_STEREO_WIDE 
    KSNODETYPE_SUM 
    KSNODETYPE_SUPERMIX 
    KSNODETYPE_SWMIDI 
    KSNODETYPE_SWSYNTH 
    KSNODETYPE_SYNTHESIZER 
    KSNODETYPE_TONE 
    KSNODETYPE_VOLUME
*/


typedef PVOID   KSDEVICE_HEADER,
                KSOBJECT_HEADER,
                KSOBJECT_BAG;




/* ===============================================================
    Method Types
*/

#define KSMETHOD_TYPE_NONE          0x00000000
#define KSMETHOD_TYPE_READ          0x00000001
#define KSMETHOD_TYPE_WRITE         0x00000002
#define KSMETHOD_TYPE_MODIFY        0x00000003
#define KSMETHOD_TYPE_SOURCE        0x00000004
#define KSMETHOD_TYPE_SEND          0x00000001
#define KSMETHOD_TYPE_SETSUPPORT    0x00000100
#define KSMETHOD_TYPE_BASICSUPPORT  0x00000200


/* ===============================================================
    Property Types
*/

#define KSPROPERTY_TYPE_GET             0x00000001
#define KSPROPERTY_TYPE_SET             0x00000002
#define KSPROPERTY_TYPE_SETSUPPORT      0x00000100
#define KSPROPERTY_TYPE_BASICSUPPORT    0x00000200
#define KSPROPERTY_TYPE_RELATIONS       0x00000400
#define KSPROPERTY_TYPE_SERIALIZESET    0x00000800
#define KSPROPERTY_TYPE_UNSERIALIZESET  0x00001000
#define KSPROPERTY_TYPE_SERIALIZERAW    0x00002000
#define KSPROPERTY_TYPE_UNSERIALIZERAW  0x00004000
#define KSPROPERTY_TYPE_SERIALIZESIZE   0x00008000
#define KSPROPERTY_TYPE_DEFAULT_VALUES  0x00010000


/* ===============================================================
    Topology Methods/Properties
*/

#define KSMETHOD_TYPE_TOPOLOGY          0x10000000
#define KSPROPERTY_TYPE_TOPOLOGY        0x10000000

/*
#define DEFINE_KS_GUID(GA,GB,GC,GD,GE,GF,GG,GH,GI,GJ,GK) \
    DEFINE_GUID(??, 0x#GA#L, 0xGB, 0xGC, 0xGD, 0xGE, 0xGF, 0xGG, 0xGH, 0xGI, 0xGJ, 0xGK) \
    "GA-GB-GC-GDGE-GFGGGHGIGJGK"
*/

/* ===============================================================
    KS Category GUIDs

    BRIDGE - 0x085AFF00L, 0x62CE, 0x11CF, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00
    CAPTURE - 0x65E8773DL, 0x8F56, 0x11D0, 0xA3, 0xB9, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96
    RENDER - 0x65E8773EL, 0x8F56, 0x11D0, 0xA3, 0xB9, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96
    MIXER - 0xAD809C00L, 0x7B88, 0x11D0, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00
    SPLITTER - 0x0A4252A0L, 0x7E70, 0x11D0, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00
    DATACOMPRESSOR - 0x1E84C900L, 0x7E70, 0x11D0, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00
    DATADECOMPRESSOR - 0x2721AE20L, 0x7E70, 0x11D0, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00
    DATATRANSFORM - 0x2EB07EA0L, 0x7E70, 0x11D0, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00
    COMMUNICATIONSTRANSFORM - 0xCF1DDA2CL, 0x9743, 0x11D0, 0xA3, 0xEE, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96
    INTERFACETRANSFORM - 0xCF1DDA2DL, 0x9743, 0x11D0, 0xA3, 0xEE, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96
    MEDIUMTRANSFORM - 0xCF1DDA2EL, 0x9743, 0x11D0, 0xA3, 0xEE, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96
    FILESYSTEM - 0x760FED5EL, 0x9357, 0x11D0, 0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96
    CLOCK - 0x53172480L, 0x4791, 0x11D0, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00
    PROXY - 0x97EBAACAL, 0x95BD, 0x11D0, 0xA3, 0xEA, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96
    QUALITY - 0x97EBAACBL, 0x95BD, 0x11D0, 0xA3, 0xEA, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96
*/

/* ===============================================================
    KSNAME GUIDs (defined also as KSSTRING_Xxx L"{...}"

    Filter - 0x9b365890L, 0x165f, 0x11d0, 0xa1, 0x95, 0x00, 0x20, 0xaf, 0xd1, 0x56, 0xe4
    Pin - 0x146F1A80L, 0x4791, 0x11D0, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00
    Clock - 0x53172480L, 0x4791, 0x11D0, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00
    Allocator - 0x642F5D00L, 0x4791, 0x11D0, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00
    TopologyNode - 0x0621061AL, 0xEE75, 0x11D0, 0xB9, 0x15, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96
*/

/* ===============================================================
    Interface GUIDs

    Standard - 0x1A8766A0L, 0x62CE, 0x11CF, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00
    FileIo - 0x8C6F932CL, 0xE771, 0x11D0, 0xB8, 0xFF, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96
*/

/* ===============================================================
    Medium Type GUIDs

    Standard - 0x4747B320L, 0x62CE, 0x11CF, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00
*/

/* ===============================================================
    Property Set GUIDs

    General - 0x1464EDA5L, 0x6A8F, 0x11D1, 0x9A, 0xA7, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96
    StreamIo - 0x65D003CAL, 0x1523, 0x11D2, 0xB2, 0x7A, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96
    MediaSeeking - 0xEE904F0CL, 0xD09B, 0x11D0, 0xAB, 0xE9, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96
    Topology - 0x720D4AC0L, 0x7533, 0x11D0, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00
    GM - 0xAF627536L, 0xE719, 0x11D2, 0x8A, 0x1D, 0x00, 0x60, 0x97, 0xD2, 0xDF, 0x5D
    Quality - 0xD16AD380L, 0xAC1A, 0x11CF, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00
    Connection - 0x1D58C920L, 0xAC9B, 0x11CF, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00
*/

/* ===============================================================
    StreamAllocator Sets

    Event set - 0x75d95571L, 0x073c, 0x11d0, 0xa1, 0x61, 0x00, 0x20, 0xaf, 0xd1, 0x56, 0xe4
    Method set - 0xcf6e4341L, 0xec87, 0x11cf, 0xa1, 0x30, 0x00, 0x20, 0xaf, 0xd1, 0x56, 0xe4
    Property set - 0xcf6e4342L, 0xec87, 0x11cf, 0xa1, 0x30, 0x00, 0x20, 0xaf, 0xd1, 0x56, 0xe4
*/

/* ===============================================================
    StreamInterface Sets

    Property set - 0x1fdd8ee1L, 0x9cd3, 0x11d0, 0x82, 0xaa, 0x00, 0x00, 0xf8, 0x22, 0xfe, 0x8a
*/

/* ===============================================================
    Clock Sets

    Property set - 0xDF12A4C0L, 0xAC17, 0x11CF, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00
    Event sets - 0x364D8E20L, 0x62C7, 0x11CF, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00
*/

/* ===============================================================
    Connection Sets

    Event set - 0x7f4bcbe0L, 0x9ea5, 0x11cf, 0xa5, 0xd6, 0x28, 0xdb, 0x04, 0xc1, 0x00, 0x00
*/

/* ===============================================================
    Time Format GUIDs

    KSTIME_FORMAT_NONE  (null guid)
    FRAME - 0x7b785570L, 0x8c82, 0x11cf, 0xbc, 0x0c, 0x00, 0xaa, 0x00, 0xac, 0x74, 0xf6
    BYTE - 0x7b785571L, 0x8c82, 0x11cf, 0xbc, 0x0c, 0x00, 0xaa, 0x00, 0xac, 0x74, 0xf6
    SAMPLE - 0x7b785572L, 0x8c82, 0x11cf, 0xbc, 0x0c, 0x00, 0xaa, 0x00, 0xac, 0x74, 0xf6
    FIELD - 0x7b785573L, 0x8c82, 0x11cf, 0xbc, 0x0c, 0x00, 0xaa, 0x00, 0xac, 0x74, 0xf6
    MEDIA_TIME - 0x7b785574L, 0x8c82, 0x11cf, 0xbc, 0x0c, 0x00, 0xaa, 0x00, 0xac, 0x74, 0xf6
*/

/* ===============================================================
    Media Type GUIDs

    NULL
    Stream -
    None -

    TODO ...
*/

/* ===============================================================
    KSMEMORY_TYPE_xxx

    WILDCARD, DONT_CARE = NULL
    SYSTEM - 0x091bb638L, 0x603f, 0x11d1, 0xb0, 0x67, 0x00, 0xa0, 0xc9, 0x06, 0x28, 0x02
    USER - 0x8cb0fc28L, 0x7893, 0x11d1, 0xb0, 0x69, 0x00, 0xa0, 0xc9, 0x06, 0x28, 0x02
    KERNEL_PAGED - 0xd833f8f8L, 0x7894, 0x11d1, 0xb0, 0x69, 0x00, 0xa0, 0xc9, 0x06, 0x28, 0x02
    KERNEL_NONPAGED - 0x4a6d5fc4L, 0x7895, 0x11d1, 0xb0, 0x69, 0x00, 0xa0, 0xc9, 0x06, 0x28, 0x02
    DEVICE_UNKNOWN - 0x091bb639L, 0x603f, 0x11d1, 0xb0, 0x67, 0x00, 0xa0, 0xc9, 0x06, 0x28, 0x02
*/

/* ===============================================================
    Enums
    (values have been checked)
*/

typedef enum
{
    KsObjectTypeDevice,
    KsObjectTypeFilterFactory,
    KsObjectTypeFilter,
    KsObjectTypePin
} KSOBJECTTYPE;

typedef enum
{
    KSSTATE_STOP,
    KSSTATE_ACQUIRE,
    KSSTATE_PAUSE,
    KSSTATE_RUN
} KSSTATE;

typedef enum
{
    KSTARGET_STATE_DISABLED,
    KSTARGET_STATE_ENABLED
} KSTARGET_STATE;

typedef enum
{
    KSRESET_BEGIN,
    KSRESET_END
} KSRESET;

typedef enum
{
    KSEVENTS_NONE,
    KSEVENTS_SPINLOCK,
    KSEVENTS_MUTEX,
    KSEVENTS_FMUTEX,
    KSEVENTS_FMUTEXUNSAFE,
    KSEVENTS_INTERRUPT,
    KSEVENTS_ERESOURCE
} KSEVENTS_LOCKTYPE;

typedef enum
{
    KSDEGRADE_STANDARD_SIMPLE,
    KSDEGRADE_STANDARD_QUALITY,
    KSDEGRADE_STANDARD_COMPUTATION,
    KSDEGRADE_STANDARD_SKIP
} KSDEGRADE_STANDARD;

typedef enum
{
    KSPIN_DATAFLOW_IN = 1,
    KSPIN_DATAFLOW_OUT
} KSPIN_DATAFLOW;

typedef enum
{
    KSPIN_COMMUNICATION_NONE,
    KSPIN_COMMUNICATION_SINK,
    KSPIN_COMMUNICATION_SOURCE,
    KSPIN_COMMUNICATION_BOTH,
    KSPIN_COMMUNICATION_BRIDGE
} KSPIN_COMMUNICATION;

typedef enum
{
    KsListEntryTail,
    KsListEntryHead
} KSLIST_ENTRY_LOCATION;

typedef enum
{
    KsStackCopyToNewLocation,
    KsStackReuseCurrentLocation,
    KsStackUseNewLocation
} KSSTACK_USE;

typedef enum
{
    KsAcquireOnly,
    KsAcquireAndRemove,
    KsAcquireOnlySingleItem,
    KsAcquireAndRemoveOnlySingleItem
} KSIRP_REMOVAL_OPERATION;

typedef enum
{
    KsInvokeOnSuccess = 1,
    KsInvokeOnError = 2,
    KsInvokeOnCancel = 4
} KSCOMPLETION_INVOCATION;



/* MOVE ME */
typedef NTSTATUS (*PFNKSCONTEXT_DISPATCH)(
    IN PVOID Context,
    IN PIRP Irp);


/* ===============================================================
    Framing
*/

typedef struct
{
} KS_FRAMING_ITEM, *PKS_FRAMING_ITEM;

typedef struct
{
} KS_FRAMING_RANGE, *PKS_FRAMING_RANGE;

typedef struct
{
    /* Obsolete */
} KS_FRAMING_RANGE_WEIGHTED, *PKS_FRAMING_RANGE_WEIGHTED;

/* ??? */
typedef struct
{
} KS_COMPRESSION, *PKS_COMPRESSION;


/* ===============================================================
    Common
*/

typedef struct
{
    GUID Set;
    ULONG Id;
    ULONG Flags;
} KSIDENTIFIER, *PKSIDENTIFIER;

typedef KSIDENTIFIER KSPROPERTY, *PKSPROPERTY;
typedef KSIDENTIFIER KSMETHOD, *PKSMETHOD;
typedef KSIDENTIFIER KSEVENT, *PKSEVENT;

typedef KSIDENTIFIER KSDEGRADE, *PKSDEGRADE;

typedef KSIDENTIFIER KSPIN_INTERFACE, *PKSPIN_INTERFACE;
typedef KSIDENTIFIER KSPIN_MEDIUM, *PKSPIN_MEDIUM;

typedef struct
{
} KSDATARANGE, *PKSDATARANGE;

typedef struct
{
} KSDATAFORMAT, *PKSDATAFORMAT;

typedef struct
{
} KSATTRIBUTE, *PKSATTRIBUTE;


/* ===============================================================
    Priorities
*/

#define KSPRIORITY_LOW          0x00000001
#define KSPRIORITY_NORMAL       0x40000000
#define KSPRIORITY_HIGH         0x80000000
#define KSPRIORITY_EXCLUSIVE    0xFFFFFFFF

typedef struct
{
    ULONG PriorityClass;
    ULONG PrioritySubClass;
} KSPRIORITY, *PKSPRIORITY;


/* ===============================================================
    Dispatch Table
    http://www.osronline.com/DDKx/stream/ks-struct_494j.htm
*/

typedef struct
{
    PDRIVER_DISPATCH DeviceIoControl;
    PDRIVER_DISPATCH Read;
    PDRIVER_DISPATCH Write;
    PDRIVER_DISPATCH Flush;
    PDRIVER_DISPATCH Close;
    PDRIVER_DISPATCH QuerySecurity;
    PDRIVER_DISPATCH SetSecurity;
    PFAST_IO_DEVICE_CONTROL FastDeviceIoControl;
    PFAST_IO_READ FastRead;
    PFAST_IO_WRITE FastWrite;
} KSDISPATCH_TABLE, *PKSDISPATCH_TABLE;

typedef struct
{
} BUS_INTERFACE_REFERENCE, *PBUS_INTERFACE_REFERENCE;

typedef struct
{
} KSCOMPONENTID, *PKSCOMPONENTID;

typedef struct
{
} KSBUFFER_ITEM, *PKSBUFFER_ITEM;

/* ===============================================================
    Properties
*/

#define KSPROPERTY_MEMBER_RANGES        0x00000001
#define KSPROPERTY_MEMBER_STEPPEDRANGES 0x00000002
#define KSPROPERTY_MEMBER_VALUES        0x00000003
#define KSPROPERTY_MEMBER_FLAG_DEFAULT  KSPROPERTY_MEMBER_RANGES

typedef struct
{
} KSPROPERTY_BOUNDS_LONG, *PKSPROPERTY_BOUNDS_LONG;

typedef struct
{
} KSPROPERTY_BOUNDS_LONGLONG, *PKSPROPERTY_BOUNDS_LONGLONG;

typedef struct
{
} KSPROPERTY_DESCRIPTION, *PKSPROPERTY_DESCRIPTION;

typedef struct
{
} KSPROPERTY_ITEM, *PKSPROPERTY_ITEM;

typedef struct
{
} KSPROPERTY_MEDIAAVAILABLE, *PKSPROPERTY_MEDIAAVAILABLE;

typedef struct
{
} KSPROPERTY_MEMBERSHEADER, *PKSPROPERTY_MEMBERSHEADER;

typedef struct
{
} KSPROPERTY_MEMBERSLIST, *PKSPROPERTY_MEMBERSLIST;

typedef struct
{
} KSPROPERTY_POSITIONS, *PKSPROPERTY_POSITIONS;

typedef struct
{
} KSPROPERTY_SERIAL, *PKSPROPERTY_SERIAL;

typedef struct
{
} KSPROPERTY_SERIALHDR, *PKSPROPERTY_SERIALHDR;

typedef struct
{
} KSPROPERTY_SET, *PKSPROPERTY_SET;

typedef struct
{
} KSPROPERTY_STEPPING_LONG, *PKSPROPERTY_STEPPING_LONG;

typedef struct
{
} KSPROPERTY_STEPPING_LONGLONG, *PKSPROPERTY_STEPPING_LONGLONG;

typedef struct
{
} KSPROPERTY_VALUES, *PKSPROPERTY_VALUES;


/* ===============================================================
    Allocator Framing
*/

typedef struct
{
} KSALLOCATOR_FRAMING, *PKSALLOCATOR_FRAMING;

typedef struct
{
} KSALLOCATOR_FRAMING_EX, *PKSALLOCATOR_FRAMING_EX;


/* ===============================================================
    Quality
*/

typedef struct
{
} KSQUALITY, *PKSQUALITY;

typedef struct
{
    HANDLE QualityManager;
    PVOID Context;
} KSQUALITY_MANAGER, *PKSQUALITY_MANAGER;

typedef struct
{
} KSRATE, *PKSRATE;

typedef struct
{
} KSRATE_CAPABILITY, *PKSRATE_CAPABILITY;

typedef struct
{
    LONGLONG Granularity;
    LONGLONG Error;
} KSRESOLUTION, *PKSRESOLUTION;

typedef struct
{
} KSRELATIVEEVENT, *PKSRELATIVEEVENT;


/* ===============================================================
    Timing
*/

typedef struct
{
    LONGLONG Time;
    ULONG Numerator;
    ULONG Denominator;
} KSTIME, *PKSTIME;

typedef struct
{
} KSCORRELATED_TIME, *PKSCORRELATED_TIME;

typedef struct
{
    KSPROPERTY Property;
    GUID SourceFormat;
    GUID TargetFormat;
    LONGLONG Time;
} KSP_TIMEFORMAT, *PKSP_TIMEFORMAT;

typedef struct
{
} KSINTERVAL, *PKSINTERVAL;

typedef struct
{
} KSFRAMETIME, *PKSFRAMETIME;


/* ===============================================================
    Clocks
*/

typedef struct
{
} KSCLOCK, *PKSCLOCK, *PKSDEFAULTCLOCK; /* OK ? */

typedef struct
{
} KSCLOCK_CREATE, *PKSCLOCK_CREATE;

typedef struct
{
} KSCLOCK_FUNCTIONTABLE, *PKSCLOCK_FUNCTIONTABLE;


/* ===============================================================
    Objects ??? SORT ME!
*/

typedef struct
{
} KSOBJECT_CREATE, *PKSOBJECT_CREATE;

typedef struct
{
} KSOBJECT_CREATE_ITEM, *PKSOBJECT_CREATE_ITEM;

typedef VOID (*PFNKSITEMFREECALLBACK)(
    IN  PKSOBJECT_CREATE_ITEM CreateItem);

typedef struct
{
} KSMULTIPLE_ITEM, *PKSMULTIPLE_ITEM;

typedef struct
{
} KSQUERYBUFFER, *PKSQUERYBUFFER;

typedef struct
{
} KSERROR, *PKSERROR;

typedef struct
{
} KSDPC_ITEM, *PKSDPC_ITEM;


/* ===============================================================
    Methods
*/

typedef struct
{
} KSMETHOD_SET, *PKSMETHOD_SET;

typedef struct
{
} KSMETHOD_ITEM, *PKSMETHOD_ITEM;

typedef struct
{
} KSFASTMETHOD_ITEM, *PKSFASTMETHOD_ITEM;


/* ===============================================================
    Nodes
*/

typedef struct
{
} KSP_NODE, *PKSP_NODE;

typedef struct
{
    KSMETHOD Method;
    ULONG NodeID;
    ULONG Reserved;
} KSM_NODE, *PKSM_NODE;

typedef struct
{
} KSE_NODE, *PKSE_NODE;

typedef struct
{
} KSNODE_CREATE, *PKSNODE_CREATE;


/* ===============================================================
    Properties?
*/

typedef struct
{
} KSFASTPROPERTY_ITEM, *PKSFASTPROPERTY_ITEM;


/* ===============================================================
    Events
*/

typedef struct
{
} KSEVENT_TIME_MARK, *PKSEVENT_TIME_MARK;

typedef struct
{
} KSEVENT_TIME_INTERVAL, *PKSEVENT_TIME_INTERVAL;

typedef struct
{
} KSEVENT_SET, *PKSEVENT_SET;

typedef struct
{
} KSEVENT_ITEM, *PKSEVENT_ITEM;

typedef struct _KSEVENT_ENTRY
{
} KSEVENT_ENTRY, *PKSEVENT_ENTRY;

typedef struct
{
} KSEVENTDATA, *PKSEVENTDATA;


/* ===============================================================
    Pins
*/

typedef struct
{
} KSPIN_DISPATCH, *PKSPIN_DISPATCH;

typedef struct
{
} KSAUTOMATION_TABLE, *PKSAUTOMATION_TABLE;

typedef struct
{
} KSPIN_DESCRIPTOR, *PKSPIN_DESCRIPTOR;

/* TODO */
/* This is just to shut the compiler up so DON'T USE IT! */
typedef void (*PFNKSINTERSECTHANDLER)(void);
typedef void (*PFNKSINTERSECTHANDLEREX)(void);

typedef struct
{
    const KSPIN_DISPATCH* Dispatch;
    const KSAUTOMATION_TABLE* AutomationTable;
    KSPIN_DESCRIPTOR PinDescriptor;
    ULONG Flags;
    ULONG InstancesPossible;
    ULONG InstancesNecessary;
    const KSALLOCATOR_FRAMING_EX* AllocatorFraming;
    PFNKSINTERSECTHANDLEREX IntersectHandler;
} KSPIN_DESCRIPTOR_EX, *PKSPIN_DESCRIPTOR_EX;

/* TODO */
#define KSPIN_FLAG_DISPATCH_LEVEL_PROCESSING
#define KSPIN_FLAG_CRITICAL_PROCESSING
#define KSPIN_FLAG_HYPERCRITICAL_PROCESSING
#define KSPIN_FLAG_ASYNCHRONOUS_PROCESSING
#define KSPIN_FLAG_DO_NOT_INITIATE_PROCESSING
#define KSPIN_FLAG_INITIATE_PROCESSING_ON_EVERY_ARRIVAL
#define KSPIN_FLAG_FRAMES_NOT_REQUIRED_FOR_PROCESSING
#define KSPIN_FLAG_ENFORCE_FIFO
#define KSPIN_FLAG_GENERATE_MAPPINGS
#define KSPIN_FLAG_DISTINCT_TRAILING_EDGE
#define KSPIN_FLAG_PROCESS_IN_RUN_STATE_ONLY
#define KSPIN_FLAG_SPLITTER
#define KSPIN_FLAG_USE_STANDARD_TRANSPORT
#define KSPIN_FLAG_DO_NOT_USE_STANDARD_TRANSPORT
#define KSPIN_FLAG_FIXED_FORMAT
#define KSPIN_FLAG_GENERATE_EOS_EVENTS
#define KSPIN_FLAG_RENDERER
#define KSPIN_FLAG_SOME_FRAMES_REQUIRED_FOR_PROCESSING
#define KSPIN_FLAG_PROCESS_IF_ANY_IN_RUN_STATE
#define KSPIN_FLAG_DENY_USERMODE_ACCESS
#define KSPIN_FLAG_IMPLEMENT_CLOCK

typedef struct
{
    const KSPIN_DESCRIPTOR_EX* Descriptor;
    KSOBJECT_BAG Bag;
    PVOID Context;
    ULONG Id;
    KSPIN_COMMUNICATION Communication;
    BOOLEAN ConnectionIsExternal;
    KSPIN_INTERFACE ConnectionInterface;
    KSPIN_MEDIUM ConnectionMedium;
    KSPRIORITY ConnectionPriority;
    PKSDATAFORMAT ConnectionFormat;
    PKSMULTIPLE_ITEM AttributeList;
    ULONG StreamHeaderSize;
    KSPIN_DATAFLOW DataFlow;
    KSSTATE DeviceState;
    KSRESET ResetState;
    KSSTATE ClientState;
} KSPIN, *PKSPIN;

typedef struct
{
    KSPIN_INTERFACE Interface;
    KSPIN_MEDIUM Medium;
    ULONG PinId;
    HANDLE PinToHandle;
    KSPRIORITY Priority;
} KSPIN_CONNECT, *PKSPIN_CONNECT;

typedef struct
{
} KSP_PIN, *PKSP_PIN;

typedef struct
{
} KSPIN_PHYSICALCONNECTION, *PKSPIN_PHYSICALCONNECTION;


/* ===============================================================
    Topology
*/

typedef struct
{
    ULONG FromNode;
    ULONG FromNodePin;
    ULONG ToNode;
    ULONG ToNodePin;
} KSTOPOLOGY_CONNECTION, *PKSTOPOLOGY_CONNECTION;

typedef struct
{
    ULONG CategoriesCount;
    const GUID* Categories;
    ULONG TopologyNodesCount;
    const GUID* TopologyNodes;
    ULONG TopologyConnectionsCount;
    const KSTOPOLOGY_CONNECTION* TopologyConnections;
    const GUID* TopologyNodesNames;
    ULONG Reserved;
} KSTOPOLOGY, *PKSTOPOLOGY;


/* ===============================================================
    ??? SORT ME
*/

/* TODO */
typedef void* UNKNOWN;

typedef PVOID (*PFNKSDEFAULTALLOCATE)(
    IN  PVOID Context);

typedef PVOID (*PFNKSDEFAULTFREE)(
    IN  PVOID Context,
    IN  PVOID Buffer);

typedef PVOID (*PFNKSINITIALIZEALLOCATOR)(
    IN  PVOID InitialContext,
    IN  PKSALLOCATOR_FRAMING AllocatorFraming,
    OUT PVOID* Context);

typedef PVOID (*PFNKSDELETEALLOCATOR)(
    IN  PVOID Context);


typedef NTSTATUS (*PFNKSALLOCATOR)(
    IN  PIRP Irp,
    IN  ULONG BufferSize,
    IN  BOOL InputOperation);

typedef NTSTATUS (*PFNKSHANDLER)(
    IN  PIRP Irp,
    IN  PKSIDENTIFIER Request,
    IN  OUT PVOID Data);

typedef BOOLEAN (*PFNKSFASTHANDLER)(
    IN  PFILE_OBJECT FileObject,
    IN  PKSIDENTIFIER UNALIGNED Request,
    IN  ULONG RequestLength,
    IN  OUT PVOID UNALIGNED Data,
    IN  ULONG DataLength,
    OUT PIO_STATUS_BLOCK IoStatus);

typedef NTSTATUS (*PFNKSADDEVENT)(
    IN  PIRP Irp,
    IN  PKSEVENTDATA EventData,
    IN  struct _KSEVENT_ENTRY* EventEntry);

typedef NTSTATUS (*PFNKINTERSECTHANDLEREX)(
    IN  PVOID Context,
    IN  PIRP Irp,
    IN  PKSP_PIN Pin,
    IN  PKSDATARANGE DataRange,
    IN  PKSDATARANGE MatchingDataRange,
    IN  ULONG DataBufferSize,
    OUT PVOID Data OPTIONAL,
    OUT PULONG DataSize);

typedef UNKNOWN PFNALLOCATORE_ALLOCATEFRAME;
typedef UNKNOWN PFNALLOCATOR_FREEFRAME;

/*
typedef struct
{
    PFNALLOCATOR_ALLOCATEFRAME AllocateFrame;
    PFNALLOCATOR_FREEFRAME FreeFrame;
}
*/

typedef struct
{
    KSALLOCATOR_FRAMING Framing;
    ULONG AllocatedFrames;
    ULONG Reserved;
} KSSTREAMALLOCATOR_STATUS, *PKSSTREAMALLOCATOR_STATUS;

typedef struct
{
    KSALLOCATOR_FRAMING_EX Framing;
    ULONG AllocatedFrames;
    ULONG Reserved;
} KSSTREAMALLOCATOR_STATUS_EX, *PKSSTREAMALLOCATOR_STATUS_EX;

typedef struct
{
    ULONG Size;
    ULONG TypeSpecificFlags;
    KSTIME PresentationTime;
    LONGLONG Duration;
    ULONG FrameExtent;
    ULONG DataUsed;
    PVOID Data;
    ULONG OptionsFlags;
} KSSTREAM_HEADER, *PKSSTREAM_HEADER;



/* ===============================================================
    XP / DX8
*/

typedef struct
{
    /* TODO */
} KSPROCESSPIN, *PKSPROCESSPIN;

typedef struct
{
    PKSPROCESSPIN* Pins;
    ULONG Count;
} KSPROCESSPIN_INDEXENTRY, *PKSPROCESSPIN_INDEXENTRY;


/* ===============================================================
    Device Dispatch
*/

typedef struct
{
    /* TODO */
} KSDEVICE, *PKSDEVICE;

typedef NTSTATUS (*PFNKSDEVICECREATE)(
    IN PKSDEVICE Device);

typedef NTSTATUS (*PFNKSDEVICEPNPSTART)(
    IN PKSDEVICE Device,
    IN PIRP Irp,
    IN PCM_RESOURCE_LIST TranslatedResourceList OPTIONAL,
    IN PCM_RESOURCE_LIST UntranslatedResourceList OPTIONAL);

typedef NTSTATUS (*PFNKSDEVICE)(
    IN PKSDEVICE Device);

typedef NTSTATUS (*PFNKSDEVICEIRP)(
    IN PKSDEVICE Device,
    IN PIRP Irp);

typedef VOID (*PFNKSDEVICEIRPVOID)(
    IN PKSDEVICE Device,
    IN PIRP Irp);

typedef NTSTATUS (*PFNKSDEVICEQUERYCAPABILITIES)(
    IN PKSDEVICE Device,
    IN PIRP Irp,
    IN OUT PDEVICE_CAPABILITIES Capabilities);

typedef NTSTATUS (*PFNKSDEVICEQUERYPOWER)(
    IN PKSDEVICE Device,
    IN PIRP Irp,
    IN DEVICE_POWER_STATE DeviceTo,
    IN DEVICE_POWER_STATE DeviceFrom,
    IN SYSTEM_POWER_STATE SystemTo,
    IN SYSTEM_POWER_STATE SystemFrom,
    IN POWER_ACTION Action);

typedef VOID (*PFNKSDEVICESETPOWER)(
    IN PKSDEVICE Device,
    IN PIRP Irp,
    IN DEVICE_POWER_STATE To,
    IN DEVICE_POWER_STATE From);

typedef struct _KSDEVICE_DISPATCH
{
    PFNKSDEVICECREATE Add;
    PFNKSDEVICEPNPSTART Start;
    PFNKSDEVICE PostStart;
    PFNKSDEVICEIRP QueryStop;
    PFNKSDEVICEIRPVOID CancelStop;
    PFNKSDEVICEIRPVOID Stop;
    PFNKSDEVICEIRP QueryRemove;
    PFNKSDEVICEIRPVOID CancelRemove;
    PFNKSDEVICEIRPVOID Remove;
    PFNKSDEVICEQUERYCAPABILITIES QueryCapabilities;
    PFNKSDEVICEIRPVOID SurpriseRemoval;
    PFNKSDEVICEQUERYPOWER Querypower;
    PFNKSDEVICESETPOWER SetPower;
} KSDEVICE_DISPATCH, *PKSDEVICE_DISPATCH;


/* ===============================================================
    Filter Dispatch
*/

typedef struct
{
} KSFILTER, *PKSFILTER;

typedef NTSTATUS (*PFNKSFILTERIRP)(
    IN PKSFILTER Filter,
    IN PIRP Irp);

typedef NTSTATUS (*PFNKSFILTERPROCESS)(
    IN PKSFILTER FIlter,
    IN PKSPROCESSPIN_INDEXENTRY ProcessPinsIndex);

typedef NTSTATUS (*PFNKSFILTERVOID)(
    IN PKSFILTER Filter);

typedef struct _KSFILTER_DISPATCH
{
    PFNKSFILTERIRP Create;
    PFNKSFILTERIRP Close;
    PFNKSFILTERPROCESS Process;
    PFNKSFILTERVOID Reset;
} KSFILTER_DISPATCH, *PKSFILTER_DISPATCH;

typedef struct {
  const KSAUTOMATION_TABLE*  AutomationTable;
  const GUID*  Type;
  const GUID*  Name;
} KSNODE_DESCRIPTOR, *PKSNODE_DESCRIPTOR;

typedef struct {
  const KSFILTER_DISPATCH*  Dispatch;
  const KSAUTOMATION_TABLE*  AutomationTable;
  ULONG  Version;
  ULONG  Flags;
  const GUID*  ReferenceGuid;
  ULONG  PinDescriptorsCount;
  ULONG  PinDescriptorSize;
  const KSPIN_DESCRIPTOR_EX*  PinDescriptors;
  ULONG  CategoriesCount;
  const GUID*  Categories;
  ULONG  NodeDescriptorsCount;
  ULONG  NodeDescriptorSize;
  const KSNODE_DESCRIPTOR*  NodeDescriptors;
  ULONG  ConnectionsCount;
  const KSTOPOLOGY_CONNECTION*  Connections;
  const KSCOMPONENTID*  ComponentId;
} KSFILTER_DESCRIPTOR, *PKSFILTER_DESCRIPTOR;

typedef struct
{
  const KSDEVICE_DISPATCH*  Dispatch;
  ULONG  FilterDescriptorsCount;
  const  KSFILTER_DESCRIPTOR*const* FilterDescriptors;
} KSDEVICE_DESCRIPTOR, *PKSDEVICE_DESCRIPTOR;

/* ===============================================================
    Minidriver Callbacks
*/

typedef NTSTATUS (*KStrMethodHandler)(
    IN  PIRP Irp,
    IN  PKSIDENTIFIER Request,
    IN  OUT PVOID Data);

typedef NTSTATUS (*KStrSupportHandler)(
    IN  PIRP Irp,
    IN  PKSIDENTIFIER Request,
    IN  OUT PVOID Data);


/* ===============================================================
    Allocator Functions
*/

KSDDKAPI NTSTATUS NTAPI
KsCreateAllocator(
    IN  HANDLE ConnectionHandle,
    IN  PKSALLOCATOR_FRAMING AllocatorFraming,
    OUT PHANDLE AllocatorHandle);

KSDDKAPI NTSTATUS NTAPI
KsCreateDefaultAllocator(
    IN  PIRP Irp);

KSDDKAPI NTSTATUS NTAPI
KsValidateAllocatorCreateRequest(
    IN  PIRP Irp,
    OUT PKSALLOCATOR_FRAMING* AllocatorFraming);

KSDDKAPI NTSTATUS NTAPI
KsCreateDefaultAllocatorEx(
    IN  PIRP Irp,
    IN  PVOID InitializeContext OPTIONAL,
    IN  PFNKSDEFAULTALLOCATE DefaultAllocate OPTIONAL,
    IN  PFNKSDEFAULTFREE DefaultFree OPTIONAL,
    IN  PFNKSINITIALIZEALLOCATOR InitializeAllocator OPTIONAL,
    IN  PFNKSDELETEALLOCATOR DeleteAllocator OPTIONAL);

KSDDKAPI NTSTATUS NTAPI
KsValidateAllocatorFramingEx(
    IN  PKSALLOCATOR_FRAMING_EX Framing,
    IN  ULONG BufferSize,
    IN  const KSALLOCATOR_FRAMING_EX* PinFraming);


/* ===============================================================
    Clock Functions
*/

typedef BOOLEAN (*PFNKSSETTIMER)(
    IN  PVOID Context,
    IN  PKTIMER Timer,
    IN  LARGE_INTEGER DueTime,
    IN  PKDPC Dpc);

typedef BOOLEAN (*PFNKSCANCELTIMER)(
    IN  PVOID Context,
    IN  PKTIMER Timer);

typedef LONGLONG (FASTCALL *PFNKSCORRELATEDTIME)(
    IN  PVOID Context,
    OUT PLONGLONG SystemTime);

KSDDKAPI NTSTATUS NTAPI
KsCreateClock(
    IN  HANDLE ConnectionHandle,
    IN  PKSCLOCK_CREATE ClockCreate,
    OUT PHANDLE ClockHandle);

KSDDKAPI NTSTATUS NTAPI
KsCreateDefaultClock(
    IN  PIRP Irp,
    IN  PKSDEFAULTCLOCK DefaultClock);

KSDDKAPI NTSTATUS NTAPI
KsAllocateDefaultClock(
    OUT PKSDEFAULTCLOCK* DefaultClock);

KSDDKAPI NTSTATUS NTAPI
KsAllocateDefaultClockEx(
    OUT PKSDEFAULTCLOCK* DefaultClock,
    IN  PVOID Context OPTIONAL,
    IN  PFNKSSETTIMER SetTimer OPTIONAL,
    IN  PFNKSCANCELTIMER CancelTimer OPTIONAL,
    IN  PFNKSCORRELATEDTIME CorrelatedTime OPTIONAL,
    IN  const KSRESOLUTION* Resolution OPTIONAL,
    IN  ULONG Flags);

KSDDKAPI VOID NTAPI
KsFreeDefaultClock(
    IN  PKSDEFAULTCLOCK DefaultClock);

KSDDKAPI NTSTATUS NTAPI
KsValidateClockCreateRequest(
    IN  PIRP Irp,
    OUT PKSCLOCK_CREATE* ClockCreate);

KSDDKAPI KSSTATE NTAPI
KsGetDefaultClockState(
    IN  PKSDEFAULTCLOCK DefaultClock);

KSDDKAPI VOID NTAPI
KsSetDefaultClockState(
    IN  PKSDEFAULTCLOCK DefaultClock,
    IN  KSSTATE State);

KSDDKAPI LONGLONG NTAPI
KsGetDefaultClockTime(
    IN  PKSDEFAULTCLOCK DefaultClock);

KSDDKAPI VOID NTAPI
KsSetDefaultClockTime(
    IN  PKSDEFAULTCLOCK DefaultClock,
    IN  LONGLONG Time);


/* ===============================================================
    Method Functions
*/

/* Method sets - TODO: Make into macros! */

#if 0
VOID
KSMETHOD_SET_IRP_STORAGE(
    IN  IRP Irp);

VOID
KSMETHOD_ITEM_IRP_STORAGE(
    IN  IRP Irp);

VOID
KSMETHOD_TYPE_IRP_STORAGE(
    IN  IRP Irp);
#endif

KSDDKAPI NTSTATUS NTAPI
KsMethodHandler(
    IN  PIRP Irp,
    IN  ULONG MethodSetsCount,
    IN  PKSMETHOD_SET MethodSet);

KSDDKAPI NTSTATUS NTAPI
KsMethodHandlerWithAllocator(
    IN  PIRP Irp,
    IN  ULONG MethodSetsCount,
    IN  PKSMETHOD_SET MethodSet,
    IN  PFNKSALLOCATOR Allocator OPTIONAL,
    IN  ULONG MethodItemSize OPTIONAL);

KSDDKAPI BOOLEAN NTAPI
KsFastMethodHandler(
    IN  PFILE_OBJECT FileObject,
    IN  PKSMETHOD UNALIGNED Method,
    IN  ULONG MethodLength,
    IN  OUT PVOID UNALIGNED Data,
    IN  ULONG DataLength,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN  ULONG MethodSetsCount,
    IN  const KSMETHOD_SET* MethodSet);


/* ===============================================================
    Property Functions
*/

KSDDKAPI NTSTATUS NTAPI
KsPropertyHandler(
    IN  PIRP Irp,
    IN  ULONG PropertySetsCount,
    IN  const KSPROPERTY_SET* PropertySet);

KSDDKAPI NTSTATUS NTAPI
KsPropertyHandlerWithAllocator(
    IN  PIRP Irp,
    IN  ULONG PropertySetsCount,
    IN  PKSPROPERTY_SET PropertySet,
    IN  PFNKSALLOCATOR Allocator OPTIONAL,
    IN  ULONG PropertyItemSize OPTIONAL);

KSDDKAPI NTSTATUS NTAPI
KsUnserializeObjectPropertiesFromRegistry(
    IN  PFILE_OBJECT FileObject,
    IN  HANDLE ParentKey OPTIONAL,
    IN  PUNICODE_STRING RegistryPath OPTIONAL);

KSDDKAPI BOOLEAN NTAPI
KsFastPropertyHandler(
    IN  PFILE_OBJECT FileObject,
    IN  PKSPROPERTY UNALIGNED Property,
    IN  ULONG PropertyLength,
    IN  OUT PVOID UNALIGNED Data,
    IN  ULONG DataLength,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN  ULONG PropertySetsCount,
    IN  const KSPROPERTY_SET* PropertySet);


/* ===============================================================
    Event Functions
*/

KSDDKAPI NTSTATUS NTAPI
KsGenerateEvent(
    IN  PKSEVENT_ENTRY EntryEvent);

KSDDKAPI NTSTATUS NTAPI
KsEnableEventWithAllocator(
    IN  PIRP Irp,
    IN  ULONG EventSetsCount,
    IN  PKSEVENT_SET EventSet,
    IN  OUT PLIST_ENTRY EventsList OPTIONAL,
    IN  KSEVENTS_LOCKTYPE EventsFlags OPTIONAL,
    IN  PVOID EventsLock OPTIONAL,
    IN  PFNKSALLOCATOR Allocator OPTIONAL,
    IN  ULONG EventItemSize OPTIONAL);

KSDDKAPI NTSTATUS NTAPI
KsGenerateDataEvent(
    IN  PKSEVENT_ENTRY EventEntry,
    IN  ULONG DataSize,
    IN  PVOID Data);

KSDDKAPI NTSTATUS NTAPI
KsEnableEvent(
    IN  PIRP Irp,
    IN  ULONG EventSetsCount,
    IN  KSEVENT_SET* EventSet,
    IN  OUT PLIST_ENTRY EventsList OPTIONAL,
    IN  KSEVENTS_LOCKTYPE EventsFlags OPTIONAL,
    IN  PVOID EventsLock OPTIONAL);

KSDDKAPI VOID NTAPI
KsDiscardEvent(
    IN  PKSEVENT_ENTRY EventEntry);

KSDDKAPI NTSTATUS NTAPI
KsDisableEvent(
    IN  PIRP Irp,
    IN  OUT PLIST_ENTRY EventsList,
    IN  KSEVENTS_LOCKTYPE EventsFlags,
    IN  PVOID EventsLock);

KSDDKAPI VOID NTAPI
KsFreeEventList(
    IN  PFILE_OBJECT FileObject,
    IN  OUT PLIST_ENTRY EventsList,
    IN  KSEVENTS_LOCKTYPE EVentsFlags,
    IN  PVOID EventsLock);


/* ===============================================================
    Topology Functions
*/

KSDDKAPI NTSTATUS NTAPI
KsValidateTopologyNodeCreateRequest(
    IN  PIRP Irp,
    IN  PKSTOPOLOGY Topology,
    OUT PKSNODE_CREATE* NodeCreate);

KSDDKAPI NTSTATUS NTAPI
KsCreateTopologyNode(
    IN  HANDLE ParentHandle,
    IN  PKSNODE_CREATE NodeCreate,
    IN  ACCESS_MASK DesiredAccess,
    OUT PHANDLE NodeHandle);

KSDDKAPI NTSTATUS NTAPI
KsTopologyPropertyHandler(
    IN  PIRP Irp,
    IN  PKSPROPERTY Property,
    IN  OUT PVOID Data,
    IN  const KSTOPOLOGY* Topology);



/* ===============================================================
    Connectivity Functions
*/

KSDDKAPI NTSTATUS NTAPI
KsCreatePin(
    IN  HANDLE FilterHandle,
    IN  PKSPIN_CONNECT Connect,
    IN  ACCESS_MASK DesiredAccess,
    OUT PHANDLE ConnectionHandle);

KSDDKAPI NTSTATUS NTAPI
KsValidateConnectRequest(
    IN  PIRP Irp,
    IN  ULONG DescriptorsCount,
    IN  KSPIN_DESCRIPTOR* Descriptor,
    OUT PKSPIN_CONNECT* Connect);

KSDDKAPI NTSTATUS NTAPI
KsPinPropertyHandler(
    IN  PIRP Irp,
    IN  PKSPROPERTY Property,
    IN  OUT PVOID Data,
    IN  ULONG DescriptorsCount,
    IN  const KSPIN_DESCRIPTOR* Descriptor);

KSDDKAPI NTSTATUS NTAPI
KsPinDataIntersection(
    IN  PIRP Irp,
    IN  PKSPIN Pin,
    OUT PVOID Data,
    IN  ULONG DescriptorsCount,
    IN  const KSPIN_DESCRIPTOR* Descriptor,
    IN  PFNKSINTERSECTHANDLER IntersectHandler);

KSDDKAPI NTSTATUS NTAPI
KsPinDataIntersectionEx(
    IN  PIRP Irp,
    IN  PKSP_PIN Pin,
    OUT PVOID Data,
    IN  ULONG DescriptorsCount,
    IN  const KSPIN_DESCRIPTOR* Descriptor,
    IN  ULONG DescriptorSize,
    IN  PFNKSINTERSECTHANDLEREX IntersectHandler OPTIONAL,
    IN  PVOID HandlerContext OPTIONAL);

/* Does this belong here? */

KSDDKAPI NTSTATUS NTAPI
KsHandleSizedListQuery(
    IN  PIRP Irp,
    IN  ULONG DataItemsCount,
    IN  ULONG DataItemSize,
    IN  const VOID* DataItems);


/* ===============================================================
    IRP Helper Functions
*/

typedef NTSTATUS (*PFNKSIRPLISTCALLBACK)(
    IN  PIRP Irp,
    IN  PVOID Context);

KSDDKAPI NTSTATUS NTAPI
KsAcquireResetValue(
    IN  PIRP Irp,
    OUT KSRESET* ResetValue);

KSDDKAPI VOID NTAPI
KsAddIrpToCancelableQueue(
    IN  OUT PLIST_ENTRY QueueHead,
    IN  PKSPIN_LOCK SpinLock,
    IN  PIRP Irp,
    IN  KSLIST_ENTRY_LOCATION ListLocation,
    IN  PDRIVER_CANCEL DriverCancel OPTIONAL);

KSDDKAPI NTSTATUS NTAPI
KsAddObjectCreateItemToDeviceHeader(
    IN  KSDEVICE_HEADER Header,
    IN  PDRIVER_DISPATCH Create,
    IN  PVOID Context,
    IN  PWCHAR ObjectClass,
    IN  PSECURITY_DESCRIPTOR SecurityDescriptor);

KSDDKAPI NTSTATUS NTAPI
KsAddObjectCreateItemToObjectHeader(
    IN  KSOBJECT_HEADER Header,
    IN  PDRIVER_DISPATCH Create,
    IN  PVOID Context,
    IN  PWCHAR ObjectClass,
    IN  PSECURITY_DESCRIPTOR SecurityDescriptor);

KSDDKAPI NTSTATUS NTAPI
KsAllocateDeviceHeader(
    OUT KSDEVICE_HEADER* Header,
    IN  ULONG ItemsCount,
    IN  PKSOBJECT_CREATE_ITEM ItemsList OPTIONAL);

KSDDKAPI NTSTATUS NTAPI
KsAllocateExtraData(
    IN  PIRP Irp,
    IN  ULONG ExtraSize,
    OUT PVOID* ExtraBuffer);

KSDDKAPI NTSTATUS NTAPI
KsAllocateObjectCreateItem(
    IN  KSDEVICE_HEADER Header,
    IN  PKSOBJECT_CREATE_ITEM CreateItem,
    IN  BOOL AllocateEntry,
    IN  PFNKSITEMFREECALLBACK ItemFreeCallback OPTIONAL);

KSDDKAPI NTSTATUS NTAPI
KsAllocateObjectHeader(
    OUT PVOID Header,
    IN  ULONG ItemsCount,
    IN  PKSOBJECT_CREATE_ITEM ItemsList OPTIONAL,
    IN  PIRP Irp,
    IN  KSDISPATCH_TABLE* Table);

KSDDKAPI VOID NTAPI
KsCancelIo(
    IN  OUT PLIST_ENTRY QueueHead,
    IN  PKSPIN_LOCK SpinLock);

KSDDKAPI VOID NTAPI
KsCancelRoutine(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

KSDDKAPI NTSTATUS NTAPI
KsDefaultDeviceIoCompletion(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

/* ELSEWHERE
KSDDKAPI ULONG NTAPI
KsDecrementCountedWorker(
    IN  PKSWORKER Worker);
*/

KSDDKAPI BOOLEAN NTAPI
KsDispatchFastIoDeviceControlFailure(
    IN  PFILE_OBJECT FileObject,
    IN  BOOLEAN Wait,
    IN  PVOID InputBuffer  OPTIONAL,
    IN  ULONG InputBufferLength,
    OUT PVOID OutputBuffer  OPTIONAL,
    IN  ULONG OutputBufferLength,
    IN  ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN  PDEVICE_OBJECT DeviceObject);   /* always return false */

KSDDKAPI BOOLEAN NTAPI
KsDispatchFastReadFailure(
    IN  PFILE_OBJECT FileObject,
    IN  PLARGE_INTEGER FileOffset,
    IN  ULONG Length,
    IN  BOOLEAN Wait,
    IN  ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN  PDEVICE_OBJECT DeviceObject);   /* always return false */

/* This function does the same as the above */
#define KsDispatchFastWriteFailure KsDispatchFastReadFailure

KSDDKAPI NTSTATUS NTAPI
KsDispatchInvalidDeviceRequest(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

KSDDKAPI NTSTATUS NTAPI
KsDispatchIrp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

KSDDKAPI NTSTATUS NTAPI
KsDispatchSpecificMethod(
    IN  PIRP Irp,
    IN  PFNKSHANDLER Handler);

KSDDKAPI NTSTATUS NTAPI
KsDispatchSpecificProperty(
    IN  PIRP Irp,
    IN  PFNKSHANDLER Handler);

KSDDKAPI NTSTATUS NTAPI
KsForwardAndCatchIrp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PFILE_OBJECT FileObject,
    IN  KSSTACK_USE StackUse);

KSDDKAPI NTSTATUS NTAPI
KsForwardIrp(
    IN  PIRP Irp,
    IN  PFILE_OBJECT FileObject,
    IN  BOOLEAN ReuseStackLocation);

KSDDKAPI VOID NTAPI
KsFreeDeviceHeader(
    IN  KSDEVICE_HEADER Header);

KSDDKAPI VOID NTAPI
KsFreeObjectHeader(
    IN  PVOID Header);

KSDDKAPI NTSTATUS NTAPI
KsGetChildCreateParameter(
    IN  PIRP Irp,
    OUT PVOID* CreateParameter);

KSDDKAPI NTSTATUS NTAPI
KsMoveIrpsOnCancelableQueue(
    IN  OUT PLIST_ENTRY SourceList,
    IN  PKSPIN_LOCK SourceLock,
    IN  OUT PLIST_ENTRY DestinationList,
    IN  PKSPIN_LOCK DestinationLock OPTIONAL,
    IN  KSLIST_ENTRY_LOCATION ListLocation,
    IN  PFNKSIRPLISTCALLBACK ListCallback,
    IN  PVOID Context);

KSDDKAPI NTSTATUS NTAPI
KsProbeStreamIrp(
    IN  PIRP Irp,
    IN  ULONG ProbeFlags,
    IN  ULONG HeaderSize);

KSDDKAPI NTSTATUS NTAPI
KsQueryInformationFile(
    IN  PFILE_OBJECT FileObject,
    OUT PVOID FileInformation,
    IN  ULONG Length,
    IN  FILE_INFORMATION_CLASS FileInformationClass);

KSDDKAPI ACCESS_MASK NTAPI
KsQueryObjectAccessMask(
    IN KSOBJECT_HEADER Header);

KSDDKAPI PKSOBJECT_CREATE_ITEM NTAPI
KsQueryObjectCreateItem(
    IN KSOBJECT_HEADER Header);

KSDDKAPI NTSTATUS NTAPI
KsReadFile(
    IN  PFILE_OBJECT FileObject,
    IN  PKEVENT Event OPTIONAL,
    IN  PVOID PortContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN  ULONG Length,
    IN  ULONG Key OPTIONAL,
    IN  KPROCESSOR_MODE RequestorMode);

KSDDKAPI VOID NTAPI
KsReleaseIrpOnCancelableQueue(
    IN  PIRP Irp,
    IN  PDRIVER_CANCEL DriverCancel OPTIONAL);

KSDDKAPI PIRP NTAPI
KsRemoveIrpFromCancelableQueue(
    IN  OUT PLIST_ENTRY QueueHead,
    IN  PKSPIN_LOCK SpinLock,
    IN  KSLIST_ENTRY_LOCATION ListLocation,
    IN  KSIRP_REMOVAL_OPERATION RemovalOperation);

KSDDKAPI VOID NTAPI
KsRemoveSpecificIrpFromCancelableQueue(
    IN  PIRP Irp);

KSDDKAPI NTSTATUS NTAPI
KsSetInformationFile(
    IN  PFILE_OBJECT FileObject,
    IN  PVOID FileInformation,
    IN  ULONG Length,
    IN  FILE_INFORMATION_CLASS FileInformationClass);

KSDDKAPI NTSTATUS NTAPI
KsSetMajorFunctionHandler(
    IN  PDRIVER_OBJECT DriverObject,
    IN  ULONG MajorFunction);

KSDDKAPI NTSTATUS NTAPI
KsStreamIo(
    IN  PFILE_OBJECT FileObject,
    IN  PKEVENT Event OPTIONAL,
    IN  PVOID PortContext OPTIONAL,
    IN  PIO_COMPLETION_ROUTINE CompletionRoutine OPTIONAL,
    IN  PVOID CompletionContext OPTIONAL,
    IN  KSCOMPLETION_INVOCATION CompletionInvocationFlags OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN  OUT PVOID StreamHeaders,
    IN  ULONG Length,
    IN  ULONG Flags,
    IN  KPROCESSOR_MODE RequestorMode);

KSDDKAPI NTSTATUS NTAPI
  KsWriteFile(
    IN  PFILE_OBJECT FileObject,
    IN  PKEVENT Event OPTIONAL,
    IN  PVOID PortContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN  PVOID Buffer,
    IN  ULONG Length,
    IN  ULONG Key OPTIONAL,
    IN  KPROCESSOR_MODE RequestorMode);


/* ===============================================================
    Worker Management Functions
*/

KSDDKAPI NTSTATUS NTAPI
KsRegisterWorker(
    IN  WORK_QUEUE_TYPE WorkQueueType,
    OUT PKSWORKER* Worker);

KSDDKAPI VOID NTAPI
KsUnregisterWorker(
    IN  PKSWORKER Worker);

KSDDKAPI NTSTATUS NTAPI
KsRegisterCountedWorker(
    IN  WORK_QUEUE_TYPE WorkQueueType,
    IN  PWORK_QUEUE_ITEM CountedWorkItem,
    OUT PKSWORKER* Worker);

KSDDKAPI ULONG NTAPI
KsDecrementCountedWorker(
    IN  PKSWORKER Worker);

KSDDKAPI ULONG NTAPI
KsIncrementCountedWorker(
    IN  PKSWORKER Worker);

KSDDKAPI NTSTATUS NTAPI
KsQueueWorkItem(
    IN  PKSWORKER Worker,
    IN  PWORK_QUEUE_ITEM WorkItem);


/* ===============================================================
    Resources / Images
*/

KSDDKAPI NTSTATUS NTAPI
KsLoadResource(
    IN  PVOID ImageBase,
    IN  POOL_TYPE PoolType,
    IN  ULONG_PTR ResourceName,
    IN  ULONG ResourceType,
    OUT PVOID* Resource,
    OUT PULONG ResourceSize);

/* TODO: Implement
KSDDKAPI NTSTATUS NTAPI
KsGetImageNameAndResourceId(
    IN  HANDLE RegKey,
    OUT PUNICODE_STRING ImageName,
    OUT PULONG_PTR ResourceId,
    OUT PULONG ValueType);

KSDDKAPI NTSTATUS NTAPI
KsMapModuleName(
    IN  PDEVICE_OBJECT PhysicalDeviceObject,
    IN  PUNICODE_STRING ModuleName,
    OUT PUNICODE_STRING ImageName,
    OUT PULONG_PTR ResourceId,
    OUT PULONG ValueType);
*/


/* ===============================================================
    Misc. Helper Functions
*/

KSDDKAPI NTSTATUS NTAPI
KsCacheMedium(
    IN  PUNICODE_STRING SymbolicLink,
    IN  PKSPIN_MEDIUM Medium,
    IN  DWORD PinDirection);

KSDDKAPI NTSTATUS NTAPI
KsDefaultDispatchPnp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

KSDDKAPI VOID NTAPI
KsSetDevicePnpAndBaseObject(
    IN  KSDEVICE_HEADER Header,
    IN  PDEVICE_OBJECT PnpDeviceObject,
    IN  PDEVICE_OBJECT BaseDevice);

KSDDKAPI NTSTATUS NTAPI
KsDefaultDispatchPower(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp);

KSDDKAPI VOID NTAPI
KsSetPowerDispatch(
    IN  KSOBJECT_HEADER Header,
    IN  PFNKSCONTEXT_DISPATCH PowerDispatch OPTIONAL,
    IN  PVOID PowerContext OPTIONAL);

KSDDKAPI NTSTATUS NTAPI
KsReferenceBusObject(
    IN  KSDEVICE_HEADER Header);

KSDDKAPI VOID NTAPI
KsDereferenceBusObject(
    IN  KSDEVICE_HEADER Header);

KSDDKAPI NTSTATUS NTAPI
KsFreeObjectCreateItem(
    IN  KSDEVICE_HEADER Header,
    IN  PUNICODE_STRING CreateItem);

KSDDKAPI NTSTATUS NTAPI
KsFreeObjectCreateItemsByContext(
    IN  KSDEVICE_HEADER Header,
    IN  PVOID Context);

VOID
KsNullDriverUnload(
    IN  PDRIVER_OBJECT DriverObject);

KSDDKAPI PDEVICE_OBJECT NTAPI
KsQueryDevicePnpObject(
    IN  KSDEVICE_HEADER Header);

KSDDKAPI VOID NTAPI
KsRecalculateStackDepth(
    IN  KSDEVICE_HEADER Header,
    IN  BOOLEAN ReuseStackLocation);

KSDDKAPI VOID NTAPI
KsSetTargetDeviceObject(
    IN  KSOBJECT_HEADER Header,
    IN  PDEVICE_OBJECT TargetDevice OPTIONAL);

KSDDKAPI VOID NTAPI
KsSetTargetState(
    IN  KSOBJECT_HEADER Header,
    IN  KSTARGET_STATE TargetState);

KSDDKAPI NTSTATUS NTAPI
KsSynchronousIoControlDevice(
    IN  PFILE_OBJECT FileObject,
    IN  KPROCESSOR_MODE RequestorMode,
    IN  DWORD IoControl,
    IN  PVOID InBuffer,
    IN  ULONG InSize,
    OUT PVOID OutBuffer,
    IN  ULONG OUtSize,
    OUT PULONG BytesReturned);


/* ===============================================================
    AVStream Functions (XP / DirectX 8)
    NOT IMPLEMENTED YET
    http://www.osronline.com/ddkx/stream/avstream_5q9f.htm
*/

KSDDKAPI NTSTATUS NTAPI
KsInitializeDriver(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING  RegistryPath,
    IN const KSDEVICE_DESCRIPTOR  *Descriptor OPTIONAL
    );

#if 0
typedef void (*PFNKSFILTERFACTORYPOWER)(
    IN  PKSFILTERFACTORY FilterFactory,
    IN  DEVICE_POWER_STATE State);

KSDDKAPI NTSTATUS NTAPI
_KsEdit(
    IN  KSOBJECT_BAG ObjectBag,
    IN  OUT PVOID* PointerToPointerToItem,
    IN  ULONG NewSize,
    IN  ULONG OldSize,
    IN  ULONG Tag);

VOID
KsAcquireControl(
    IN  PVOID Object)
{
}

VOID
KsAcquireDevice(
    IN  PKSDEVICE Device)
{
}

NTSTATUS
KsAddDevice(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PDEVICE_OBJECT PhysicalDeviceObject)
{
}

VOID
KsAddEvent(
    IN  PVOID Object,
    IN  PKSEVENT_ENTRY EventEntry)
{
}

NTSTATUS
KsAddItemToObjectBag(
    IN  KSOBJECT_BAG ObjectBag,
    IN  PVOID Item,
    IN  PFNKSFREE Free OPTIONAL)
{
}

NTSTATUS
KsAllocateObjectBag(
    IN  PKSDEVICE Device,
    OUT KSOBJECT_BAG* ObjectBag)
{
}

VOID
KsCompletePendingRequest(
    IN  PIRP Irp)
{
}

NTSTATUS
KsCopyObjectBagItems(
    IN  KSOBJECT_BAG ObjectBagDestination,
    IN  KSOBJECT_BAG ObjectBagSource)
{
}

NTSTATUS
KsCreateDevice(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PDEVICE_OBJECT PhysicalDeviceObject,
    IN  const KSDEVICE_DESCRIPTOR* Descriptor OPTIONAL,
    IN  ULONG ExtensionSize OPTIONAL,
    OUT PKSDEVICE* Device OPTIONAL)
{
}

NTSTATUS
KsCreateFilterFactory(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  const KSFILTER_DESCRIPTOR* Descriptor,
    IN  PWCHAR RefString OPTIONAL,
    IN  PSECURITY_DESCRIPTOR SecurityDescriptor OPTIONAL,
    IN  ULONG CreateItemFlags,
    IN  PFNKSFILTERFACTORYPOWER SleepCallback OPTIONAL,
    IN  PFNKSFILTERFACTORYPOWER WakeCallback OPTIONAL,
    OUT PKFSFILTERFACTORY FilterFactory OPTIONAL)
{
}

NTSTATUS
KsDefaultAddEventHandler(
    IN  PIRP Irp,
    IN  PKSEVENTDATA EventData,
    IN  OUT PKSEVENT_ENTRY EventEntry)
{
}

NTSTATUS
KsDeleteFilterFactory(
    IN  PKSFILTERFACTORY FilterFactory)
{
}

ULONG
KsDeviceGetBusData(
    IN  PKSDEVICE Device,
    IN  ULONG DataType,
    IN  PVOID Buffer,
    IN  ULONG Offset,
    IN  ULONG Length)
{
}

PKSFILTERFACTORY
KsDeviceGetFirstChildFilterFactory(
    IN  PKSDEVICE Device)
{
}

PUNKNOWN
KsDeviceGetOuterUnknown(
    IN  PKSDEVICE Device)
{
}

VOID
KsDeviceRegisterAdapterObject(
    IN  PKSDEVICE Device,
    IN  PADAPTER_OBJECT AdapterObject,
    IN  ULONG MaxMappingByteCount,
    IN  ULONG MappingTableStride)
{
}

KSDDKAPI PUNKNOWN NTAPI
KsDeviceRegisterAggregatedClientUnknown(
    IN  PKSDEVICE Device,
    IN  PUNKNOWN ClientUnknown);

ULONG
KsDeviceSetBusData(
    IN  PKSDEVICE Device,
    IN  ULONG DataType,
    IN  PVOID Buffer,
    IN  ULONG Offset,
    IN  ULONG Length)
{
}

#define KsDiscard(object, pointer) \
    KsRemoveItemFromObjectBag(object->Bag, pointer, TRUE)

VOID
KsFilterAcquireControl(
    IN  PKSFILTER Filter)
{
}

VOID
KsFilterAcquireProcessingMutex(
    IN  PKSFILTER Filter);

VOID
KsFilterAddEvent(
    IN  PKSFILTER Filter,
    IN  PKSEVENT_ENTRY EventEntry)
{
}

KSDDKAPI NTSTATUS NTAPI
KsFilterAddTopologyConnections(
    IN  PKSFILTER Filter,
    IN  ULONG NewConnectionsCount,
    IN  const KSTOPOLOGY_CONNECTION* NewTopologyConnections);

VOID
KsFilterAttemptProcessing(
    IN  PKSFILTER Filter,
    IN  BOOLEAN Asynchronous);

KSDDKAPI NTSTATUS NTAPI
KsFilterCreateNode(
    IN  PKSFILTER Filter,
    IN  const KSNODE_DESCRIPTOR* NodeDescriptor,
    OUT PULONG NodeID);

KSDDKAPI NTSTATUS NTAPI
KsFilterCreatePinFactory(
    IN  PKSFILTER Filter,
    IN  const KSPIN_DESCRIPTOR_EX* PinDescriptor,
    OUT PULONG PinID);

PKSDEVICE __inline
KsFilterFactoryGetDevice(
    IN  PKSFILTERFACTORY FilterFactory);

/* etc. */
#endif /* avstream */

#ifdef __cplusplus
}
#endif

#endif
