/*************************************************************
 * File  :  hello_rtclite.c
 * Module:  RTNLite Engine API Demo Application.
 *
 * This demo showcases basic usage of the RTNLite Engine API
 * for joining a channel, sending, and receiving media.
 *************************************************************/

 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>
 #include <unistd.h> // For usleep, getopt
 #include <signal.h> // For signal handling
 #include <time.h>   // For basic timestamping
 #include <sys/time.h> // For gettimeofday
 #include <strings.h>
 #include "rtnlite_engine_api.h" // Our main API
 #include "pacer.h" // For controlling frame send rate
 #include "3rd/file_parser/include/file_parser.h" // For reading media files
 
 // Demo utilities (conceptual, replace with actual implementations)
 
 #define DEFAULT_VIDEO_FILE "../../../media/h264SampleFrames/" // Needs to be a directory for file_parser
 #define DEFAULT_AUDIO_FILE "../../../media/opusSampleFrames/" // Needs to be a directory for file_parser
 #define DEFAULT_VIDEO_FPS 25
 #define DEFAULT_AUDIO_FRAME_DURATION_MS 20 // For Opus
 
 // Application-specific context
 typedef struct {
     rtnlite_service_t    service_handle;
     rtnlite_connection_t connection_handle;
 
     // Configuration from command line
     char local_user_id[128];
     char signaling_url[256];
     char room_id[256];
     char video_file_path[256];
     char audio_file_path[256];
     int  video_fps;
 
     // Media sending state
     void *video_file_parser;
     void *audio_file_parser;
     void *pacer_handle;
     uint8_t* video_buffer; // Reusable buffer for video frames
     size_t video_buffer_size;
     uint8_t* audio_buffer; // Reusable buffer for audio frames
     size_t audio_buffer_size;
 
    // Application state - using sig_atomic_t to ensure atomic signal handling
    volatile sig_atomic_t app_quit_flag;
    volatile bool rtc_connected_flag;
     int           sent_video_frames;
     int           sent_audio_frames;
 
 } app_context_t;
 
 static app_context_t g_app_ctx; // Global application context for simplicity in demo
 
 // --- Helper Functions ---
 
 // Function to get current time in microseconds
 static uint64_t get_current_time_us() {
     struct timeval tv;
     gettimeofday(&tv, NULL);
     return (uint64_t)tv.tv_sec * 1000000LL + (uint64_t)tv.tv_usec;
 }
 
 static void print_usage(const char* app_name) {
     printf("Usage: %s -u <user_id> -s <signaling_url> -r <room_id> [-v <video_file_dir>] [-a <audio_file_dir>] [-f <fps>]\n", app_name);
     printf("Options:\n");
     printf("  -u <user_id>         : Local user identifier (required).\n");
     printf("  -s <signaling_url>   : WebSocket signaling server URL (e.g., wss://host:port/path) (required).\n");
     printf("  -r <room_id>         : Room/Channel ID to join (required).\n");
     printf("  -v <video_file_dir>  : Directory path for H.264 frame files (default: %s).\n", DEFAULT_VIDEO_FILE);
     printf("  -a <audio_file_dir>  : Directory path for Opus frame files (default: %s).\n", DEFAULT_AUDIO_FILE);
     printf("  -f <fps>             : Video frames per second for sending (default: %d).\n", DEFAULT_VIDEO_FPS);
     printf("  -h                   : Show this help message.\n");
 }
 
 static int parse_arguments(int argc, char* argv[], app_context_t* ctx) {
    int opt;
    
    // 设置默认参数值
    strcpy(ctx->local_user_id, "harry");          // 默认用户ID: harry
    strcpy(ctx->signaling_url, "wss://localhost:9081"); // 默认信令服务器: wss://localhost:9081
    strcpy(ctx->room_id, "666");                   // 默认房间ID: 666
    strcpy(ctx->video_file_path, "out/send_video.h264"); // 默认视频文件
    strcpy(ctx->audio_file_path, "out/send_audio_16k_1ch.pcm"); // 默认音频文件
    ctx->video_fps = 20;                             // 默认帧率: 20fps

    // 不需要跟踪是否设置这些参数，因为已有默认值

    while ((opt = getopt(argc, argv, "u:s:r:v:a:f:h")) != -1) {
         switch (opt) {
             case 'u':
                 strncpy(ctx->local_user_id, optarg, sizeof(ctx->local_user_id) - 1);
                 // 用户ID已设置
                 break;
             case 's':
                 strncpy(ctx->signaling_url, optarg, sizeof(ctx->signaling_url) - 1);
                 // 信令URL已设置
                 break;
             case 'r':
                 strncpy(ctx->room_id, optarg, sizeof(ctx->room_id) - 1);
                 // 房间ID已设置
                 break;
             case 'v':
                 strncpy(ctx->video_file_path, optarg, sizeof(ctx->video_file_path) - 1);
                 break;
             case 'a':
                 strncpy(ctx->audio_file_path, optarg, sizeof(ctx->audio_file_path) - 1);
                 break;
             case 'f':
                 ctx->video_fps = atoi(optarg);
                 if (ctx->video_fps <= 0 || ctx->video_fps > 60) {
                     fprintf(stderr, "Invalid FPS value. Using default %d.\n", DEFAULT_VIDEO_FPS);
                     ctx->video_fps = DEFAULT_VIDEO_FPS;
                 }
                 break;
             case 'h':
                 print_usage(argv[0]);
                 return -1; // Indicate help was shown, exit
             default:
                 print_usage(argv[0]);
                 return -2; // Indicate parse error
         }
     }
 
     // 不再要求必须提供这些参数，因为已经有默认值
    printf("Using connection parameters:\n");
    printf("  User ID: %s\n", ctx->local_user_id);
    printf("  Signaling URL: %s\n", ctx->signaling_url);
    printf("  Room ID: %s\n", ctx->room_id);
    printf("  Video file: %s\n", ctx->video_file_path);
    printf("  Audio file: %s\n", ctx->audio_file_path);
    printf("  Video FPS: %d\n", ctx->video_fps);
     return 0;
 }
 
 static void app_signal_handler(int sig) {
    static volatile sig_atomic_t signal_count = 0;
    
    printf("捕获信号 %d. 正在退出...\n", sig);
    g_app_ctx.app_quit_flag = 1; // 使用1而不是true，因为它是sig_atomic_t类型
    
    // 如果连续两次收到信号，则强制退出
    signal_count++;
    if (signal_count >= 2) {
        printf("强制退出程序...\n");
        _exit(1); // 强制退出，不执行清理
    }
 }
 
 static int initialize_media_sources(app_context_t* ctx) {
    printf("Initializing video source: %s\n", ctx->video_file_path);
    // For H264 video, parser_cfg can often be NULL if not specifically needed by the parser implementation.
    ctx->video_file_parser = create_file_parser(MEDIA_FILE_TYPE_H264, ctx->video_file_path, NULL);
    if (!ctx->video_file_parser) {
        fprintf(stderr, "Failed to create video file parser for path: %s\n", ctx->video_file_path);
        return -1;
    }
    printf("Initializing audio source: %s\n", ctx->audio_file_path);
    parser_cfg_t audio_p_cfg;
    memset(&audio_p_cfg, 0, sizeof(parser_cfg_t));
    
    // 根据文件名确定音频类型和参数
    media_file_type_e audio_type = MEDIA_FILE_TYPE_OPUS; // 默认值
    
    // 获取文件后缀名以判断类型
    const char* file_ext = strrchr(ctx->audio_file_path, '.');
    if (file_ext) {
        file_ext++; // 跳过点号
        
        if (strcasecmp(file_ext, "pcm") == 0) {
            audio_type = MEDIA_FILE_TYPE_PCM;
            // 从文件名中提取采样率和通道数信息
            int sample_rate = 16000; // 默认值
            int channels = 1;         // 默认值
            
            // 尝试从文件名中提取信息，格式如“send_audio_16k_1ch.pcm”
            const char* sr_pos = strstr(ctx->audio_file_path, "_");
            if (sr_pos) {
                sr_pos = strstr(sr_pos + 1, "_");
                if (sr_pos && strstr(sr_pos, "k_")) {
                    // 尝试提取采样率，例如“16k”
                    int parsed_rate = atoi(sr_pos + 1);
                    if (parsed_rate > 0) {
                        sample_rate = parsed_rate * 1000; // 转换为赫兹
                    }
                    
                    // 尝试提取通道数，例如“1ch”
                    const char* ch_pos = strstr(sr_pos, "ch");
                    if (ch_pos && ch_pos > sr_pos + 2) {
                        int parsed_ch = atoi(ch_pos - 1);
                        if (parsed_ch > 0) {
                            channels = parsed_ch;
                        }
                    }
                }
            }
            
            printf("  PCM格式检测到采样率: %d Hz, 通道数: %d\n", sample_rate, channels);
            audio_p_cfg.u.audio_cfg.sampleRateHz = sample_rate;
            audio_p_cfg.u.audio_cfg.numberOfChannels = channels;
            audio_p_cfg.u.audio_cfg.framePeriodMs = 20; // PCM帧大小
        } else if (strcasecmp(file_ext, "opus") == 0) {
            audio_type = MEDIA_FILE_TYPE_OPUS;
            audio_p_cfg.u.audio_cfg.sampleRateHz = 48000; // Opus默认采样率
            audio_p_cfg.u.audio_cfg.numberOfChannels = 1;  // 单声道
            audio_p_cfg.u.audio_cfg.framePeriodMs = 20;    // Opus帧间隔
            printf("  Opus格式, 采样率: 48000 Hz, 通道数: 1\n");
        } else if (strcasecmp(file_ext, "aac") == 0) {
            audio_type = MEDIA_FILE_TYPE_AACLC;
            audio_p_cfg.u.audio_cfg.sampleRateHz = 44100; // AAC常用采样率
            audio_p_cfg.u.audio_cfg.numberOfChannels = 2;  // 立体声
            audio_p_cfg.u.audio_cfg.framePeriodMs = 20;    // 帧间隔
            printf("  AAC格式, 采样率: 44100 Hz, 通道数: 2\n");
        }
    }
    
    ctx->audio_file_parser = create_file_parser(audio_type, ctx->audio_file_path, &audio_p_cfg);
    if (!ctx->audio_file_parser) {
        fprintf(stderr, "Failed to create audio file parser for path: %s\n", ctx->audio_file_path);
        destroy_file_parser(ctx->video_file_parser); // Clean up already created parser
        ctx->video_file_parser = NULL;
        return -1;
    }
    
    // 初始化pacer以控制音视频发送速率
    printf("Initializing media pacer\n");
    // 设置音视频发送间隔，单位为微秒
    uint32_t audio_send_interval_us = 20000; // 20ms for audio
    uint32_t video_send_interval_us = 33333; // ~30fps for video
    
    ctx->pacer_handle = pacer_create(audio_send_interval_us, video_send_interval_us);
    if (!ctx->pacer_handle) {
        fprintf(stderr, "Failed to create media pacer\n");
        destroy_file_parser(ctx->video_file_parser);
        destroy_file_parser(ctx->audio_file_parser);
        ctx->video_file_parser = NULL;
        ctx->audio_file_parser = NULL;
        return -1;
    }
    printf("Media pacer initialized successfully\n");
    
    return 0;
}

static void cleanup_media_sources(app_context_t* ctx) {
    if (ctx->video_file_parser) {
        destroy_file_parser(ctx->video_file_parser);
        ctx->video_file_parser = NULL;
    }
    if (ctx->audio_file_parser) {
        destroy_file_parser(ctx->audio_file_parser);
        ctx->audio_file_parser = NULL;
    }
    if (ctx->pacer_handle) {
        pacer_destroy(ctx->pacer_handle);
        ctx->pacer_handle = NULL;
    }
    free(ctx->video_buffer);
    ctx->video_buffer = NULL;
    ctx->video_buffer_size = 0;
    free(ctx->audio_buffer);
    ctx->audio_buffer = NULL;
    ctx->audio_buffer_size = 0;
}


 
 static int send_video_frame_from_file(app_context_t* ctx) {
     frame_t file_frame;
     if (file_parser_obtain_frame(ctx->video_file_parser, &file_frame) < 0) {
         // fprintf(stderr, "Video EOF or error obtaining frame.\n");
         // Looping removed as file_parser_reset is not available in current file_parser.h
         return -1; 
     }
 
     // Ensure buffer is large enough
     if (file_frame.len > ctx->video_buffer_size) {
         uint8_t* new_buf = (uint8_t*)realloc(ctx->video_buffer, file_frame.len);
         if (!new_buf) {
             fprintf(stderr, "Failed to realloc video buffer.\n");
             file_parser_release_frame(ctx->video_file_parser, &file_frame);
             return RTNLITE_ERR_NO_MEMORY;
         }
         ctx->video_buffer = new_buf;
         ctx->video_buffer_size = file_frame.len;
     }
     memcpy(ctx->video_buffer, file_frame.ptr, file_frame.len);
     
     rtnlite_video_frame_t frame_to_send;
     memset(&frame_to_send, 0, sizeof(rtnlite_video_frame_t));
     frame_to_send.codec_type = RTNLITE_VIDEO_CODEC_H264;
     frame_to_send.frame_type = file_frame.u.video.is_key_frame ? RTNLITE_VIDEO_FRAME_TYPE_KEY : RTNLITE_VIDEO_FRAME_TYPE_DELTA;
     frame_to_send.buffer = ctx->video_buffer;
     frame_to_send.length = file_frame.len;
     // frame_to_send.width = ... ; // If available from file_parser metadata
     // frame_to_send.height = ...;
     frame_to_send.render_time_ms = get_current_time_us() / 1000;
 
 
     int ret = rtnlite_send_video_frame(ctx->connection_handle, &frame_to_send);
     file_parser_release_frame(ctx->video_file_parser, &file_frame);
     if (ret == RTNLITE_ERR_OK) {
         ctx->sent_video_frames++;
     }
     return ret;
 }
 
 static int send_audio_frame_from_file(app_context_t* ctx) {
     frame_t file_frame;
     if (file_parser_obtain_frame(ctx->audio_file_parser, &file_frame) < 0) {
         // fprintf(stderr, "Audio EOF or error obtaining frame.\n");
         // Looping removed as file_parser_reset is not available in current file_parser.h
         return -1;
     }
 
     if (file_frame.len > ctx->audio_buffer_size) {
         uint8_t* new_buf = (uint8_t*)realloc(ctx->audio_buffer, file_frame.len);
         if (!new_buf) {
             fprintf(stderr, "Failed to realloc audio buffer.\n");
             file_parser_release_frame(ctx->audio_file_parser, &file_frame);
             return RTNLITE_ERR_NO_MEMORY;
         }
         ctx->audio_buffer = new_buf;
         ctx->audio_buffer_size = file_frame.len;
     }
     memcpy(ctx->audio_buffer, file_frame.ptr, file_frame.len);
 
     rtnlite_audio_frame_t frame_to_send;
     memset(&frame_to_send, 0, sizeof(rtnlite_audio_frame_t));
     frame_to_send.codec_type = RTNLITE_AUDIO_CODEC_OPUS;
     frame_to_send.buffer = ctx->audio_buffer;
     frame_to_send.length = file_frame.len;
     frame_to_send.samples_per_channel = 960; // Specific to 20ms Opus @ 48kHz
     frame_to_send.sample_rate_hz = 48000;
     frame_to_send.num_channels = 1;
     frame_to_send.render_time_ms = get_current_time_us() / 1000;
 
     int ret = rtnlite_send_audio_frame(ctx->connection_handle, &frame_to_send);
     file_parser_release_frame(ctx->audio_file_parser, &file_frame);
      if (ret == RTNLITE_ERR_OK) {
         ctx->sent_audio_frames++;
     }
     return ret;
 }
 
 
 // --- Callback Implementations for RTNLite Engine ---
 
 static void app_on_service_error(rtnlite_service_t service, rtnlite_error_e err, const char* msg, void* user_data) {
    fprintf(stderr, "SERVICE ERROR: code %d (%s), msg: %s\n", err, rtnlite_err_to_str(err), msg);
    (void)service;
    (void)user_data;
    // Potentially set quit flag for critical service errors
    // if (app) app->app_quit_flag = true;
 }

 static void app_on_join_channel_success(rtnlite_connection_t connection, const char* channel_id, const char* user_id, int elapsed_ms, void* user_data) {
    // app_context_t* app = (app_context_t*)user_data; // Unused variable
    printf("APP_CB: Successfully joined channel '%s' as user '%s'. Elapsed: %d ms.\n", channel_id, user_id, elapsed_ms);
    (void)connection;
    (void)user_data; // Mark user_data as unused if app was its only use here
 }
 
 static void app_on_leave_channel(rtnlite_connection_t connection, rtnlite_error_e reason, void* user_data) {
     app_context_t* app = (app_context_t*)user_data;
     printf("APP_CB: Left channel. Reason: %s (%d).\n", rtnlite_err_to_str(reason), reason);
     app->rtc_connected_flag = false;
     (void)connection;
 }
 
 static void app_on_connection_state_changed(rtnlite_connection_t connection, rtnlite_connection_state_e state, rtnlite_error_e reason, void* user_data) {
     app_context_t* app = (app_context_t*)user_data;
     printf("APP_CB: Connection state changed to: %d. Reason: %s (%d).\n", state, rtnlite_err_to_str(reason), reason);
     (void)connection;

     switch (state) {
         case RTNLITE_CONNECTION_STATE_CONNECTED:
             app->rtc_connected_flag = true;
             printf("*************************************************\n");
             printf(">>> RTC Connection Established! Ready for media. <<<\n");
             printf("*************************************************\n");
             break;
         case RTNLITE_CONNECTION_STATE_FAILED:
             app->rtc_connected_flag = false;
             fprintf(stderr, "APP_CB: Connection FAILED. Error: %s\n", rtnlite_err_to_str(reason));
             break;
         case RTNLITE_CONNECTION_STATE_DISCONNECTED:
             app->rtc_connected_flag = false;
             printf("APP_CB: Connection Disconnected.\n");
             break;
         default:
             break;
     }
 }
 
 static void app_on_user_joined(rtnlite_connection_t connection, const char* user_id, int elapsed_ms, void* user_data) {
     printf("APP_CB: Remote user '%s' joined. Elapsed: %d ms.\n", user_id, elapsed_ms);
     (void)connection;
     (void)user_data;
 }
 
 static void app_on_user_offline(rtnlite_connection_t connection, const char* user_id, rtnlite_error_e reason, void* user_data) {
     printf("APP_CB: Remote user '%s' went offline. Reason: %s (%d).\n", user_id, rtnlite_err_to_str(reason), reason);
     (void)connection;
     (void)user_data;
 }
 
 static void app_on_remote_video_frame(rtnlite_connection_t connection, const char* user_id, const rtnlite_video_frame_t* frame, void* user_data) {
     static int count = 0;
     if (count % 100 == 0) { // Log every 100 frames
         printf("APP_CB: Received video frame from user '%s': size=%zu, type=%d, ts=%lums\n",
               user_id, frame->length, frame->frame_type, (unsigned long)frame->render_time_ms);
     }
     count++;
     (void)connection;
     (void)user_data;
 }
 
 static void app_on_remote_audio_frame(rtnlite_connection_t connection, const char* user_id, const rtnlite_audio_frame_t* frame, void* user_data) {
     static int count = 0;
     if (count % 500 == 0) { // Log every 500 frames
         printf("APP_CB: Received audio frame from user '%s': size=%zu, ts=%lums\n",
               user_id, frame->length, (unsigned long)frame->render_time_ms);
     }
     count++;
     (void)connection;
     (void)user_data;
 }
 
 static void app_on_local_ice_candidate(rtnlite_connection_t connection, const char* candidate_json_or_sdp_line, void* user_data) {
     if (candidate_json_or_sdp_line) {
         printf("APP_CB: Local ICE candidate: %s\n", candidate_json_or_sdp_line);
     } else {
         printf("APP_CB: Local ICE candidate gathering finished.\n");
     }
     (void)connection;
     (void)user_data;
 }
 
 static void app_on_error(rtnlite_connection_t connection, rtnlite_error_e err, const char* msg, void* user_data) {
     fprintf(stderr, "APP_CB: Connection Error: code %d (%s), msg: %s\n", err, rtnlite_err_to_str(err), msg);
     (void)connection;
     (void)user_data;
 }
 
 
 // --- Main Application ---
 int main(int argc, char *argv[]) {
    memset(&g_app_ctx, 0, sizeof(app_context_t)); // Initialize global context
   
    // 显式初始化重要字段为NULL
    g_app_ctx.service_handle = NULL;
    g_app_ctx.connection_handle = NULL;
    g_app_ctx.video_file_parser = NULL;
    g_app_ctx.audio_file_parser = NULL;
    g_app_ctx.pacer_handle = NULL;
    g_app_ctx.video_buffer = NULL;
    g_app_ctx.audio_buffer = NULL;
 
     printf("RTNLite Engine Demo Application\n");
 
     if (parse_arguments(argc, argv, &g_app_ctx) != 0) {
         return 1;
     }
 
     signal(SIGINT, app_signal_handler);
     signal(SIGTERM, app_signal_handler);
 
     // 1. Initialize RTNLite Service
     rtnlite_service_config_t service_cfg;
     memset(&service_cfg, 0, sizeof(service_cfg));
     service_cfg.app_id = "hello-rtclite-app"; // Can be any string
     service_cfg.user_data = &g_app_ctx;       // Pass app context to service callbacks
 
     rtnlite_service_event_handler_t service_evt_handler;
     memset(&service_evt_handler, 0, sizeof(service_evt_handler));
     service_evt_handler.on_service_error = app_on_service_error;
     // Add other service-level handlers if any are defined in the API
 
     if (rtnlite_service_create(&service_cfg, &service_evt_handler, &g_app_ctx.service_handle) != RTNLITE_ERR_OK) {
         fprintf(stderr, "Failed to create RTNLite service.\n");
         return 1;
     }
     printf("RTNLite service created.\n");
 
     // 2. Configure and Create RTNLite Connection
     rtnlite_conn_config_t conn_cfg;
     memset(&conn_cfg, 0, sizeof(conn_cfg));
     conn_cfg.user_id = g_app_ctx.local_user_id;
     conn_cfg.signaling_url = g_app_ctx.signaling_url;
     conn_cfg.user_data = &g_app_ctx; // Pass app context to connection callbacks
 
     // Example STUN server (replace with TURN if needed for NAT traversal)
     conn_cfg.ice_server_count = 1;
     strncpy(conn_cfg.ice_servers[0].urls, "stun:stun.l.google.com:19302", sizeof(conn_cfg.ice_servers[0].urls) - 1);
     // conn_cfg.ice_servers[0].username[0] = '\0'; // No auth for public STUN
     // conn_cfg.ice_servers[0].credential[0] = '\0';
 
     rtnlite_event_handler_t conn_evt_handler;
     memset(&conn_evt_handler, 0, sizeof(conn_evt_handler));
     conn_evt_handler.on_join_channel_success = app_on_join_channel_success;
     conn_evt_handler.on_leave_channel = app_on_leave_channel;
     conn_evt_handler.on_connection_state_changed = app_on_connection_state_changed;
     conn_evt_handler.on_user_joined = app_on_user_joined;
     conn_evt_handler.on_user_offline = app_on_user_offline;
     conn_evt_handler.on_remote_video_frame = app_on_remote_video_frame;
     conn_evt_handler.on_remote_audio_frame = app_on_remote_audio_frame;
     conn_evt_handler.on_local_ice_candidate = app_on_local_ice_candidate;
     conn_evt_handler.on_error = app_on_error;
     // conn_evt_handler.on_remote_ice_candidate_added = app_on_remote_ice_candidate_added; // If needed
     // conn_evt_handler.on_network_quality = app_on_network_quality; // If implemented
 
     if (rtnlite_connection_create(g_app_ctx.service_handle, &conn_cfg, &conn_evt_handler, &g_app_ctx.connection_handle) != RTNLITE_ERR_OK) {
         fprintf(stderr, "Failed to create RTNLite connection.\n");
         rtnlite_service_destroy(g_app_ctx.service_handle);
         return 1;
     }
     printf("RTNLite connection created.\n");
 
     // 3. Initialize media sources (file parsers and pacer)
     if (initialize_media_sources(&g_app_ctx) != 0) {
         fprintf(stderr, "Failed to initialize media sources.\n");
         // Cleanup connection and service
         rtnlite_connection_destroy(g_app_ctx.connection_handle);
         rtnlite_service_destroy(g_app_ctx.service_handle);
         return 1;
     }
 
     // 4. Join Channel
    rtnlite_channel_options_t channel_opts; // Kept for API compatibility, currently minimal
    memset(&channel_opts, 0, sizeof(channel_opts));
    channel_opts.auto_subscribe_audio = true;
    channel_opts.auto_subscribe_video = true;

    printf("Joining channel '%s'...\n", g_app_ctx.room_id);
    int join_result = rtnlite_channel_join(g_app_ctx.connection_handle, NULL /*token*/, g_app_ctx.room_id, NULL /*user_id from conn_cfg*/, &channel_opts);
    if (join_result != RTNLITE_ERR_OK) {
        fprintf(stderr, "Failed to initiate channel join for room '%s'.\n", g_app_ctx.room_id);
        // 连接失败时直接退出
        return -1;
    } 
 
     // 5. Main application loop (sending media)
    printf("Entering main loop. Press Ctrl+C to quit.\n");
    uint64_t last_stats_print_time = get_current_time_us();

    while (g_app_ctx.app_quit_flag == 0) { // 使用 == 0 比较，因为现在是sig_atomic_t类型
        // 正常模式：媒体发送
        if (g_app_ctx.rtc_connected_flag) {
            if (is_time_to_send_video(g_app_ctx.pacer_handle)) {
                if (send_video_frame_from_file(&g_app_ctx) != RTNLITE_ERR_OK) {
                    // Potentially log error, parser will loop or error out
                }
            }
            if (is_time_to_send_audio(g_app_ctx.pacer_handle)) {
                 if (send_audio_frame_from_file(&g_app_ctx) != RTNLITE_ERR_OK) {
                    // Potentially log error, parser will loop or error out
                }
            }
            wait_before_next_send(g_app_ctx.pacer_handle);

            // Print stats periodically
            if (get_current_time_us() - last_stats_print_time >= 5 * 1000000) { // Every 5 seconds
                printf("STATS: Sent Video Frames: %d, Sent Audio Frames: %d\n", g_app_ctx.sent_video_frames, g_app_ctx.sent_audio_frames);
                last_stats_print_time = get_current_time_us();
            }

        } else {
            usleep(100 * 1000); // Wait if not connected
        }
    } 
 
     // 6. Cleanup
     printf("Exiting application...\n");
     
     if (g_app_ctx.connection_handle) {
         printf("Leaving channel...\n");
         rtnlite_channel_leave(g_app_ctx.connection_handle); 
         printf("Destroying connection...\n");
         rtnlite_connection_destroy(g_app_ctx.connection_handle);
         g_app_ctx.connection_handle = NULL;
     }
     
     cleanup_media_sources(&g_app_ctx);
 
     if (g_app_ctx.service_handle) {
         printf("Destroying service...\n");
         rtnlite_service_destroy(g_app_ctx.service_handle);
         g_app_ctx.service_handle = NULL;
     }
     
     printf("Cleanup complete. Bye!\n");
     return 0;
 }
