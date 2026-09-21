/*

capture a s

*/
#include "RAWtoPNG.h"

#include <spa/param/video/format-utils.h>
#include <spa/debug/types.h>
#include <spa/param/video/type-info.h>

#include <pipewire/pipewire.h>

#include <stdio.h>
#include <stdlib.h>

struct data {
        struct pw_main_loop *loop;
        struct pw_context *context;
        struct pw_core *core;
        struct pw_registry *registry;
        struct spa_hook      registry_listener;
        struct spa_hook      core_listener;

        struct pw_stream    *video_in_stream;
        struct spa_hook      video_listener;

        struct pw_stream    *audio_out_stream;

        struct pw_stream    *audio_in_stream;
        struct spa_hook      audio_listener;

        struct spa_video_info format;
        
        uint32_t target_node_id;
        int sync_seq;
};

// Callback that watches the registered nodes on the server
static void registry_event_global(void *userdata, uint32_t id,
                                  uint32_t permissions, const char *type,
                                  uint32_t version, const struct spa_dict *props)
{
        struct data *data = userdata;
        if (strcmp(type, PW_TYPE_INTERFACE_Node) != 0 || props == NULL)
        return;

        const char *name = props ? spa_dict_lookup(props, PW_KEY_NODE_NAME) : NULL;
        const char *media_class = props ? spa_dict_lookup(props, PW_KEY_MEDIA_CLASS) : NULL;

        if(!media_class) return;

        if (strcmp(media_class, "Stream/Output/Video") == 0 ||
        strcmp(media_class, "Video/Source") == 0) {

        printf("=== Video NOde found ===\n");
        printf("ID: %u\n", id);
        printf("Name: %s\n", name);

        if (name && strcasestr(name, "gamescope")){
                data->target_node_id = id;
        } else if (data->target_node_id == PW_ID_ANY) {
                data->target_node_id = id;
        }
        }
}

static const struct pw_registry_events registry_events = {
        PW_VERSION_REGISTRY_EVENTS,
        .global = registry_event_global,
};

//2. Callback to determine when the initial list of nodes has finished loading
static void on_core_done(void *userdata, uint32_t id, int seq)
{
        struct data *data = userdata;
        if (id == PW_ID_CORE && seq == data->sync_seq) {
        
        pw_main_loop_quit(data->loop);
    }
}

static const struct pw_core_events core_events = {
        PW_VERSION_CORE_EVENTS,
        .done = on_core_done,
};

//from here ultil the main funcion are the funcions assigned to capture the screenshot
static void on_process(void *userdata)
{
        struct data *data = userdata;
        struct pw_buffer *b;
        struct spa_buffer *buf;

        b = pw_stream_dequeue_buffer(data->video_in_stream);
        if (!b) return;

        buf = b->buffer;

        uint32_t size = buf->datas[0].chunk->size;
 
        if (buf->datas[0].data != NULL){
            uint32_t width  = data->format.info.raw.size.width;
            uint32_t height = data->format.info.raw.size.height;

            printf("Captured Frame! Size: %d bytes(%ux%u)\n",
                size,
                width,
                height);



            if(bgrx_to_png_file(buf->datas[0].data, width, height, "screenshot.png") == 0)
            {
                printf("Screenshot sucessfully saved in screenshot.png!\n");
            } else {
                fprintf(stderr, "Error while converting and saving image\n");
            }

            pw_stream_queue_buffer(data->video_in_stream, b);
            pw_main_loop_quit(data->loop);
            return;
        }
                


        pw_stream_queue_buffer(data->video_in_stream, b);
}
/* [on_process] */

static void on_param_changed(void *userdata, uint32_t id, const struct spa_pod *param)
{
        struct data *data = userdata;

        if (param == NULL || id != SPA_PARAM_Format)
                return;

        if (spa_format_parse(param,
                        &data->format.media_type,
                        &data->format.media_subtype) < 0)
                return;

        if (data->format.media_type != SPA_MEDIA_TYPE_video ||
            data->format.media_subtype != SPA_MEDIA_SUBTYPE_raw)
                return;

        if (spa_format_video_raw_parse(param, &data->format.info.raw) < 0)
                return;

        printf("got video format:\n");
        printf("  format: %d (%s)\n", data->format.info.raw.format,
                        spa_debug_type_find_name(spa_type_video_format,
                                data->format.info.raw.format));
        printf("  size: %dx%d\n", data->format.info.raw.size.width,
                        data->format.info.raw.size.height);
        printf("  framerate: %d/%d\n", data->format.info.raw.framerate.num,
                        data->format.info.raw.framerate.denom);

}


static const struct pw_stream_events stream_events = {
        PW_VERSION_STREAM_EVENTS,
        .param_changed = on_param_changed,
        .process = on_process,
};


int main(int argc, char *argv[])
{
        struct data data = { 0, };
        data.target_node_id = PW_ID_ANY;

        const struct spa_pod *params[1];
        uint8_t buffer[1024];
        struct spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

        pw_init(&argc, &argv);

        data.loop = pw_main_loop_new(NULL);
        data.context = pw_context_new(pw_main_loop_get_loop(data.loop), NULL, 0);
        data.core = pw_context_connect(data.context, NULL, 0);

        // ----------NODE_ID SEARCHING STEP-----------
        if (argc > 1) {
        data.target_node_id = (uint32_t)atoi(argv[1]);
        printf("Usando nó informado manualmente: %u\n", data.target_node_id);
        } else {
          pw_core_add_listener(data.core, &data.core_listener, &core_events, &data);

          data.registry = pw_core_get_registry(data.core, PW_VERSION_REGISTRY, 0);
          pw_registry_add_listener(data.registry, &data.registry_listener, &registry_events, &data);
          //sync with the server to list the existing nodes
          data.sync_seq = pw_core_sync(data.core, PW_ID_CORE, 0);
          pw_main_loop_run(data.loop);

          //clean the registry hooks
          spa_hook_remove(&data.registry_listener);
          spa_hook_remove(&data.core_listener);
          pw_proxy_destroy((struct pw_proxy *)data.registry);
        }

        printf("connected to the nods: %u\n", data.target_node_id);

        // STREAM CAPTURE STEP
        data.video_in_stream = pw_stream_new(
                        data.core,
                        "SCRENSHOT CAPTURE",
                        pw_properties_new(PW_KEY_MEDIA_TYPE, "Video", PW_KEY_MEDIA_CATEGORY, "Capture",PW_KEY_MEDIA_ROLE, "Screen",  NULL)
                      );

        pw_stream_add_listener(data.video_in_stream, &data.video_listener, &stream_events, &data);
       /* IGNORE
       
       data.audio_in_stream = pw_stream_new(
                        data.core,
                        "sound-effect-stream",
                        pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY, "Playback", PW_KEY_MEDIA_ROLE, "Music", NULL)
                      );
        //pw_stream_add_listener(data.audio_in_stream, &data.audio_listener, &stream_events, &data);

        data.audio_out_stream = pw_stream_new(
                            data.core,
                            "sound-effect-stream",
                            pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY, "Capture", NULL)
                          );

       
       */
        
        params[0] = spa_pod_builder_add_object(&b,
                SPA_TYPE_OBJECT_Format, SPA_PARAM_EnumFormat,
                SPA_FORMAT_mediaType,       SPA_POD_Id(SPA_MEDIA_TYPE_video),
                SPA_FORMAT_mediaSubtype,    SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
                SPA_FORMAT_VIDEO_format,    SPA_POD_CHOICE_ENUM_Id(7,
                                                SPA_VIDEO_FORMAT_RGB,
                                                SPA_VIDEO_FORMAT_RGB,
                                                SPA_VIDEO_FORMAT_RGBA,
                                                SPA_VIDEO_FORMAT_RGBx,
                                                SPA_VIDEO_FORMAT_BGRx,
                                                SPA_VIDEO_FORMAT_YUY2,
                                                SPA_VIDEO_FORMAT_I420),
                SPA_FORMAT_VIDEO_size,      SPA_POD_CHOICE_RANGE_Rectangle(
                                                &SPA_RECTANGLE(320, 240),
                                                &SPA_RECTANGLE(1, 1),
                                                &SPA_RECTANGLE(4096, 4096)),
                SPA_FORMAT_VIDEO_framerate, SPA_POD_CHOICE_RANGE_Fraction(
                                                &SPA_FRACTION(25, 1),
                                                &SPA_FRACTION(0, 1),
                                                &SPA_FRACTION(1000, 1)));

        pw_stream_connect(data.video_in_stream,
                          PW_DIRECTION_INPUT,
                          data.target_node_id,
                          PW_STREAM_FLAG_AUTOCONNECT |
                          PW_STREAM_FLAG_MAP_BUFFERS,
                          params, 1);

        pw_main_loop_run(data.loop);

        pw_stream_destroy(data.video_in_stream);
        //pw_stream_destroy(data.audio_in_stream);
        //pw_stream_destroy(data.audio_out_stream);
        pw_core_disconnect(data.core);
        pw_context_destroy(data.context);
        pw_main_loop_destroy(data.loop);

        return 0;
}