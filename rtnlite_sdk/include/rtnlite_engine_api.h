#ifndef RTNLITE_ENGINE_API_H
#define RTNLITE_ENGINE_API_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// 前置声明，从RTNLITE SDK引入的不透明句柄类型
typedef struct RtcPeerConnection* RTNLite_PRtcPeerConnection;
typedef struct RtcRtpTransceiver* RTNLite_PRtcRtpTransceiver;
typedef struct Frame RTNLite_Frame, *RTNLite_PFrame; // RTNLITE帧结构

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RTNLITE_API
#ifdef __cplusplus
  #define RTNLITE_API extern "C" __attribute__((visibility("default")))
#else
  #define RTNLITE_API __attribute__((visibility("default")))
#endif
#endif

// 不透明的服务句柄（全局SDK实例）
typedef struct rtnlite_service_internal_s* rtnlite_service_t;

// 不透明的连接句柄（对应一个WebRTC连接）
typedef struct rtnlite_connection_internal_s* rtnlite_connection_t;

// --- 错误码 ---
typedef enum {
    RTNLITE_ERR_OK = 0,
    RTNLITE_ERR_FAILED = 1,
    RTNLITE_ERR_INVALID_ARG = 2,
    RTNLITE_ERR_INVALID_STATE = 3,
    RTNLITE_ERR_NOT_INITIALIZED = 7,
    RTNLITE_ERR_TIMEDOUT = 10,
    RTNLITE_ERR_NO_MEMORY = 11,
    RTNLITE_ERR_NET_DOWN = 14,
    RTNLITE_ERR_SIGNALING_ERROR = 100, // 信令错误
    RTNLITE_ERR_RTNLITE_ERROR = 200,       // RTNLITE SDK通用错误
} rtnlite_error_e;

// --- 服务配置结构 ---
typedef struct {
    const char* app_id;         // 全局应用ID
    // 其他潜在的全局SDK设置
    void* user_data;            // 服务级回调的用户数据（如果有）
} rtnlite_service_config_t;

// --- 连接配置结构 ---
typedef struct {
    const char* user_id;      // 用户ID
    const char* signaling_url; // 此连接的信令服务器URL

    // ICE服务器配置
    int ice_server_count;
    struct {
        char urls[256];        // 单个URL或逗号分隔的多个URL
        char username[256];    // 用户名（如果需要认证）
        char credential[256];  // 密码（如果需要认证）
    } ice_servers[5]; // 最多5个ICE服务器

    void* user_data; // 连接回调的用户数据
} rtnlite_conn_config_t;

typedef struct {
    const char* token;      // 可选的频道加入令牌(信令服务器特定)
    bool auto_subscribe_audio; // 占位符，实际订阅由应用管理
    bool auto_subscribe_video; // 占位符
} rtnlite_channel_options_t;



// --- 媒体帧结构 ---
typedef enum {
    RTNLITE_VIDEO_CODEC_H264 = 0, // 映射到RTNLITE RTC_CODEC_H264_PROFILE_42E01F_...
    RTNLITE_VIDEO_CODEC_VP8  = 1, // 映射到RTNLITE RTC_CODEC_VP8
    RTNLITE_VIDEO_CODEC_H265 = 2, // 映射到RTNLITE RTC_CODEC_H265
} rtnlite_video_codec_type_e;

typedef enum {
    RTNLITE_VIDEO_FRAME_TYPE_KEY = 0,   // IDR帧
    RTNLITE_VIDEO_FRAME_TYPE_DELTA = 1, // P/B帧
} rtnlite_video_frame_type_e;

typedef struct {
    rtnlite_video_codec_type_e codec_type;
    rtnlite_video_frame_type_e frame_type; // 关键帧或非关键帧
    const uint8_t* buffer;
    size_t length;
    uint32_t width;  // 可选，信息用
    uint32_t height; // 可选，信息用
    uint64_t render_time_ms; // 显示时间戳
} rtnlite_video_frame_t;

typedef enum {
    RTNLITE_AUDIO_CODEC_OPUS = 0, // 映射到RTNLITE RTC_CODEC_OPUS
    RTNLITE_AUDIO_CODEC_PCM_U8,   // 映射到RTNLITE RTC_CODEC_MULAW (PCMU)
    RTNLITE_AUDIO_CODEC_PCM_A8,   // 映射到RTNLITE RTC_CODEC_ALAW (PCMA)
} rtnlite_audio_codec_type_e;

typedef struct {
    rtnlite_audio_codec_type_e codec_type;
    const uint8_t* buffer;
    size_t length;
    int samples_per_channel; // 例如，Opus 20ms @ 48kHz为960
    int sample_rate_hz;      // 例如，Opus为48000，G711为8000
    int num_channels;
    uint64_t render_time_ms; // 显示时间戳
} rtnlite_audio_frame_t;


// --- 连接状态 ---
typedef enum {
    RTNLITE_CONNECTION_STATE_DISCONNECTED = 1,
    RTNLITE_CONNECTION_STATE_CONNECTING = 2,   // ICE/信令进行中
    RTNLITE_CONNECTION_STATE_CONNECTED = 3,    // DTLS已连接，准备媒体传输
    RTNLITE_CONNECTION_STATE_RECONNECTING = 4, // 自动重连(如果RTNLITE/信令支持)
    RTNLITE_CONNECTION_STATE_FAILED = 5,
} rtnlite_connection_state_e;

// --- 网络质量 ---
typedef enum {
    RTNLITE_NETWORK_QUALITY_UNKNOWN = 0,
    RTNLITE_NETWORK_QUALITY_EXCELLENT = 1,
    RTNLITE_NETWORK_QUALITY_GOOD = 2,
    RTNLITE_NETWORK_QUALITY_POOR = 3,
    RTNLITE_NETWORK_QUALITY_BAD = 4,
    RTNLITE_NETWORK_QUALITY_VBAD = 5, // 非常差
    RTNLITE_NETWORK_QUALITY_DOWN = 6,
} rtnlite_network_quality_e;

// --- 事件处理函数结构 ---
typedef struct {
    // 连接和频道事件
    void (*on_join_channel_success)(rtnlite_connection_t connection, const char* channel_id, const char* user_id, int elapsed_ms, void* user_data);
    void (*on_leave_channel)(rtnlite_connection_t connection, rtnlite_error_e reason, void* user_data);
    void (*on_rejoin_channel_success)(rtnlite_connection_t connection, const char* channel_id, const char* user_id, int elapsed_ms, void* user_data);
    void (*on_connection_lost)(rtnlite_connection_t connection, void* user_data);
    void (*on_connection_state_changed)(rtnlite_connection_t connection,
                                        rtnlite_connection_state_e state,
                                        rtnlite_error_e reason, // 如果状态为FAILED
                                        void* user_data);

    // 远程用户事件
    void (*on_user_joined)(rtnlite_connection_t connection, const char* user_id, int elapsed_ms, void* user_data);
    void (*on_user_offline)(rtnlite_connection_t connection, const char* user_id, rtnlite_error_e reason, void* user_data);

    // 信令事件
    void (*on_local_ice_candidate)(rtnlite_connection_t connection, const char* candidate_json_or_sdp_line, void* user_data);
    void (*on_remote_ice_candidate_added)(rtnlite_connection_t connection, const char* user_id, const char* candidate_json_or_sdp_line, void* user_data);

    // 媒体事件
    void (*on_remote_video_frame)(rtnlite_connection_t connection, const char* user_id, const rtnlite_video_frame_t* frame, void* user_data);
    void (*on_remote_audio_frame)(rtnlite_connection_t connection, const char* user_id, const rtnlite_audio_frame_t* frame, void* user_data);
    
    // 网络质量
    void (*on_network_quality)(rtnlite_connection_t connection, const char* user_id, rtnlite_network_quality_e tx_quality, rtnlite_network_quality_e rx_quality, void* user_data);

    // 错误事件
    void (*on_error)(rtnlite_connection_t connection, rtnlite_error_e err, const char* msg, void* user_data);
} rtnlite_event_handler_t;

// 可能的服务级事件处理函数结构
typedef struct {
    // 服务层错误
    void (*on_service_error)(rtnlite_service_t service, rtnlite_error_e err, const char* msg, void* user_data);
    // 其他可能的服务层事件
} rtnlite_service_event_handler_t;

// --- 核心API函数 ---

// 创建服务实例
RTNLITE_API int rtnlite_service_create(const rtnlite_service_config_t* config,
                                    const rtnlite_service_event_handler_t* event_handler, // 可以为NULL
                                    rtnlite_service_t* p_service);

// 销毁服务实例
RTNLITE_API int rtnlite_service_destroy(rtnlite_service_t service);

// 创建连接
RTNLITE_API int rtnlite_connection_create(rtnlite_service_t service,
                                      const rtnlite_conn_config_t* conn_config,
                                      const rtnlite_event_handler_t* event_handler,
                                      rtnlite_connection_t* p_connection);

// 销毁连接
RTNLITE_API int rtnlite_connection_destroy(rtnlite_connection_t connection);

// 加入频道/房间
RTNLITE_API int rtnlite_channel_join(rtnlite_connection_t connection,
                                   const char* token,
                                   const char* channel_id,
                                   const char* user_id,
                                   const rtnlite_channel_options_t* options);

// 离开频道/房间
RTNLITE_API int rtnlite_channel_leave(rtnlite_connection_t connection);

// 发送视频帧
RTNLITE_API int rtnlite_send_video_frame(rtnlite_connection_t connection, const rtnlite_video_frame_t* frame);

// 发送音频帧
RTNLITE_API int rtnlite_send_audio_frame(rtnlite_connection_t connection, const rtnlite_audio_frame_t* frame);

// 启用或禁用本地视频发送
RTNLITE_API int rtnlite_enable_local_video(rtnlite_connection_t connection, bool enabled);

// 启用或禁用本地音频发送
RTNLITE_API int rtnlite_enable_local_audio(rtnlite_connection_t connection, bool enabled);

// --- 兼容层函数（用于过渡） ---

// 创建引擎（兼容旧API，内部使用service+connection）
RTNLITE_API int rtnlite_engine_create(const rtnlite_conn_config_t* config,
                                   const rtnlite_event_handler_t* event_handler,
                                   rtnlite_connection_t* p_engine);

// 销毁引擎（兼容旧API）
RTNLITE_API int rtnlite_engine_destroy(rtnlite_connection_t engine);


// --- 辅助函数 ---
// 将错误码转换为字符串表示
RTNLITE_API const char* rtnlite_err_to_str(rtnlite_error_e err);


#ifdef __cplusplus
}
#endif

#endif // RTNLITE_ENGINE_API_H
