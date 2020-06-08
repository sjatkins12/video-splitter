#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>


static void SaveFrame(AVFrame *pFrame, int width, int height, const char *file_name) {
      FILE *pFile;
      int  y;

      // Open file
      pFile=fopen(file_name, "wb");
      if(pFile==NULL)
      {
          fprintf(stderr, "Failed to open file");
          return;
      }

      // Write header
      fprintf(pFile, "P6\n%d %d\n255\n", width, height);

      // Write pixel data
      for(y=0; y<height; y++)
          fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);

      // Close file
      fclose(pFile);
}

static int get_video_info(AVFormatContext **pFormatCtx, char *filename)
{
    *pFormatCtx = avformat_alloc_context();
    if (!pFormatCtx)
        return -1;

    // Open video file
    if(avformat_open_input(pFormatCtx, filename, NULL, 0) != 0)
        return -1; // Couldn't open file    

    // Retrieve stream information
    if(avformat_find_stream_info(*pFormatCtx, NULL) < 0)
        return -1; // Couldn't find stream information

    return 0;
} 

static int find_video_stream(AVCodecParameters **pCodecParameters, AVFormatContext *pFormatCtx)
{
    int32_t video_stream;

    video_stream = -1;
    for (uint32_t stream_index = 0; stream_index < pFormatCtx->nb_streams; stream_index++)
    {
        AVCodecParameters *pLocalCodecParameters = NULL;

        pLocalCodecParameters = pFormatCtx->streams[stream_index]->codecpar;
        if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            *pCodecParameters = pLocalCodecParameters;
            return stream_index;
        }
    }
    return -1;
}

static int init_codec(AVCodecContext **pCodecCtx, AVCodecParameters *pCodecParameters)
{
    AVCodec *pCodec = NULL;

    pCodec = avcodec_find_decoder(pCodecParameters->codec_id);

    *pCodecCtx = avcodec_alloc_context3(pCodec);
    if (!*pCodecCtx) {
        return -1; // Failed to allocate memory for context
    }

    if (avcodec_parameters_to_context(*pCodecCtx, pCodecParameters) < 0) {
        return -1; // Failed to copy codec params to context
    }

    // Open codec
    if(avcodec_open2(*pCodecCtx, pCodec, NULL) < 0)
        return -1; // Could not open codec
    
    return 0;
}

static int init_frame_buffer(AVFrame *pFrameRGB, AVCodecContext *pCodecCtx)
{
    uint8_t *buffer = NULL;
    int32_t numBytes;

    // Determine required buffer size and allocate buffer
     numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width,
             pCodecCtx->height, 16);
     buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
     if (!buffer)
         return -1;

     av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer, AV_PIX_FMT_RGB24,
             pCodecCtx->width, pCodecCtx->height, 1);

    return 0;
}

static int decode_packet(AVPacket *packet, AVCodecContext *pCodecCtx, AVFrame *pFrame)
{
    int response;

    response = avcodec_send_packet(pCodecCtx, packet);
    if (response < 0) {
        fprintf(stderr, "Failed to send packet to decoder");
        return response;
    }

    while (response >= 0) {
        response = avcodec_receive_frame(pCodecCtx, pFrame);

        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            break;
        }
        else if (response < 0 ) {
            fprintf(stderr, "Error receiving frame from decoder");
            return response;
        }

        if (response >= 0) {
            return 1;
        }
    }

    return 0;
}

typedef struct SwsContext t_SwsContext;

static int capture_frames(int videoStream, AVCodecContext *pCodecCtx, AVFormatContext *pFormatCtx)
{
    AVFrame *pFrame = NULL;
    AVFrame *pFrameRGB = NULL;
    AVPacket *packet = NULL;
    t_SwsContext *sws_ctx = NULL;
    
    pFrameRGB = av_frame_alloc();
    pFrame = av_frame_alloc();
    packet = av_packet_alloc();

    if(!pFrame || !pFrameRGB || !packet)
        return -1;

    if (init_frame_buffer(pFrameRGB, pCodecCtx) < 0)
        return -1;

    sws_ctx = sws_getContext(
            pCodecCtx->width,
            pCodecCtx->height,
            pCodecCtx->pix_fmt,
            pCodecCtx->width,
            pCodecCtx->height,
            AV_PIX_FMT_RGB24,
            SWS_BILINEAR,
            NULL,
            NULL,
            NULL
            );

    int response = 0;
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        if (packet->stream_index == videoStream) {
            response = decode_packet(packet, pCodecCtx, pFrame);
            if (response < 0)
                break;
            else if (response > 0) {
                char frame_filename[1024];

                sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);

                snprintf(frame_filename, sizeof(frame_filename), "assets/%s-%d.pgm", "frame", pCodecCtx->frame_number);
                SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, frame_filename);
            }
        }
        av_packet_unref(packet);
    }

    av_packet_free(&packet);
    av_frame_free(&pFrame);
    av_free(pFrameRGB->data[0]); // Free buffer
    av_frame_free(&pFrameRGB);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: ffmpeg_tuto video_file");
    }
    AVFormatContext *pFormatCtx = NULL;

    if (get_video_info(&pFormatCtx, argv[1]) < 0)
        return -1;

    int videoStream;
    AVCodecContext *pCodecCtx = NULL;
    AVCodecParameters *pCodecParameters = NULL;

    videoStream = find_video_stream(&pCodecParameters, pFormatCtx);
    if(videoStream == -1)
        return -1; // Didn't find a video stream
    
    if (init_codec(&pCodecCtx, pCodecParameters) < 0)
        return -1;

    capture_frames(videoStream, pCodecCtx, pFormatCtx);

    avformat_close_input(&pFormatCtx);
    avcodec_free_context(&pCodecCtx);

    return 0;
}
