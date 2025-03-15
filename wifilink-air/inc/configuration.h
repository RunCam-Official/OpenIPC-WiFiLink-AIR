#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

#define RC_SET_PARAM_SYNC   1

typedef enum
{
    AIR_PARAM_INDEX_CHANNEL,
    AIR_PARAM_INDEX_TXPOWER,
} AirParam_index;

typedef enum
{
    AIR_PARAM_TYPE_INT,
    AIR_PARAM_TYPE_STRING,
} AirParamType;

typedef struct
{
    /*wfb*/
    int wfb_channel;
    int wfb_driver_txpower_override;
    int wfb_stbc;
    int wfb_ldpc;
    int wfb_mcs_index;
    /*Majestic*/
    char maj_video_codec[8];
    int maj_video_fps;
    char maj_video_size[16];
    int maj_video_bitrate;
    char maj_video_records[8];
    char maj_image_mirror[8];
    char maj_image_flip[8];
    int maj_image_rotate;
    int maj_image_contrast;
    int maj_image_hue;
    int maj_image_saturation;
    int maj_image_luminance;
    char maj_audio_enable[8];
    int maj_audio_volume;
    /*telemetry*/
    int telemetry_router;
} AirParam_s;

typedef struct 
{
    AirParam_index ap_index;
    AirParam_s air_param;
    AirParamType ap_type;
}AirParam_info;

typedef enum {
    VALID_INDEX_1,
    VALID_INDEX_2,
    VALID_INDEX_3,
    VALID_INDEX_4,
    VALID_INDEX_5,
    VALID_INDEX_MAX
} ValidIndex;

typedef struct {
    ValidIndex index;
    char resolutionInfo[50];
    char splitResolution[20];
    char splitFramerate[10];
} VideoSizeInfo;

int get_air_init_param(void);
int CardConfigurationCheck(void);
int system_value_sync_to_sdcard(void);

#endif //#define _CONFIGURATION_H_