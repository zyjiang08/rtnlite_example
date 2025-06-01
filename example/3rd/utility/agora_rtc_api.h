/*************************************************************
 * 简单的agora_rtc_api.h头文件
 * 仅用于编译，实际使用时应替换为真实的头文件
 *************************************************************/

#ifndef __AGORA_RTC_API_H__
#define __AGORA_RTC_API_H__

#ifdef __cplusplus
extern "C" {
#endif

// 音频数据类型定义
#define AUDIO_DATA_TYPE_OPUS 1
#define AUDIO_DATA_TYPE_AACLC 2
#define AUDIO_DATA_TYPE_HEAAC 3
#define AUDIO_DATA_TYPE_PCMA 4
#define AUDIO_DATA_TYPE_PCMU 5
#define AUDIO_DATA_TYPE_G722 6
#define AUDIO_DATA_TYPE_PCM 7
#define AUDIO_DATA_TYPE_AACLC_8K 8
#define AUDIO_DATA_TYPE_AACLC_16K 9

// 视频数据类型定义
#define VIDEO_DATA_TYPE_H264 1
#define VIDEO_DATA_TYPE_H265 2
#define VIDEO_DATA_TYPE_GENERIC_JPEG 3

#ifdef __cplusplus
}
#endif

#endif /* __AGORA_RTC_API_H__ */
