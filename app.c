#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/avutil.h>
#include <libavcodec/avpacket.c>
#include <libavutil/imgutils.h>

// Downloaded ffmpeg source file and configured it using ./configure from git bash, later adding those folders to mingw include directory manually
// Refer to https://trac.ffmpeg.org/wiki/CompilationGuide/MinGW

// A function to write RGB information to PPM file(idk what this format does but it has RGB info laid out in a long string in binary)
void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame);


int main(int argc, char *argv[]){
    // Registering all available file formats and codecs with library so they will be used automatically when a file with any format is opened
    // av_register_all();
    // UPDATE: This function is deprecated and is no longer in use
    
    // Initialising variable for storing file format info
    AVFormatContext *pFormatCtx = NULL;

    // Opening video file
    // The first argument reads the file header and stores the information about file format
    if(avformat_open_input(&pFormatCtx, argv[1], NULL, 0) != 0)
        return -1; // failed in opening
    
    // Check stream information and populate pFormatCtx->streams with information
    if(avformat_find_stream_info(pFormatCtx, NULL) < 0)
        return -1; // failed in finding stream info

    // Debugging function in case of error dumps info into standard error
    av_dump_format(pFormatCtx, 0, argv[1], 0);

    // pFormatCtx->streams is an array of pointers of size ..->nb_streams
    // Iterating through file i/o object to find video stream
    int i;
    AVCodecContext *pCodecCtxOrig = NULL;
    AVCodecContext *pCodecCtx = NULL;

    // Find the first video stream
    int videoStream = -1;
    for(int i = 0; i < pFormatCtx->nb_streams; i++)
        if(pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            videoStream = i;
            break;
        }
    
    // If video stream was not found
    if(videoStream == -1)
        return -1;
    
    // Get a pointer to codec context for video stream
    pCodecCtx = pFormatCtx->streams[videoStream]->codecpar;
    // The stream info about the codec is in the codec context, which contains all info about the codec used by the stream.

    // Now, finding the "actual" codec and opening it
    AVCodec *pCodec = NULL;

    // Find the decoder for video stream
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if(pCodec == NULL){
        fprintf(stderr, "Unsupported codec!\n");
        return -1;
    }
    // Copy Codec context from the video stream since we cannot use it directly
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if(avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0){
        fprintf(stderr, "Couldn't copy codec context");
        return -1; // Error copying codec context
    }

    // Open codec
    if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
    return -1; // Could not open Codec

    // STORING FRAMES
    AVFrame *pFrame = NULL;
    AVFrame *pFrameRGB = NULL;  //might be the wrong data type for this
    // Allocate video frame
    pFrame = av_frame_alloc();   
    /* Since frames will be output in the form of PPM files(stored in 24-bit RGB)
        we are going to convert our frame from its native format to RGB by ffmpeg*/
    // Allocate an AVFrame structure
    pFrameRGB = av_frame_alloc();
    if(pFrameRGB == NULL)
        return -1;

    // Creating space to place raw data while it is converted
    uint8_t *buffer = NULL;
    int numBytes;
    numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, pCodecCtx->width,
                                    pCodecCtx->height);
    buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

    /*Use avpicture_fill to associate the frame with our newly allocated buffer
        Casting to AVPicture(AVPicture *)[AVPicture is deprecated] as beginning of AVFrame struct is identical to it(avframe is superset of avpicture)*/

    // Assign appropriate parts of buffer to image planes in pFrameRGB
    av_image_fill_arrays(pFrameRGB, buffer, AV_PIX_FMT_RGB24,
                    pCodecCtx->width, pCodecCtx->height);

    // READING DATA

    // Process will be something like- read entire stream in packet-> decode it into frame-> convert and save frame
    struct SwsContext *sws_ctx = NULL;
    int frameFinished;
    AVPacket packet;
    // initialize SWS context for software scaling
    sws_ctx = sws_getContext(pCodecCtx->width,
        pCodecCtx->height,
        pCodecCtx->pix_fmt,
        pCodecCtx->width,
        pCodecCtx->height,
        AV_PIX_FMT_RGB24,
        SWS_BILINEAR,   // expands to 2
        NULL,
        NULL,
        NULL
        );

    i = 0;
    while(av_read_frame(pFormatCtx, &packet) >= 0){
        // Is this a packet from the video stream?
        if(packet.stream_index == videoStream){
            // Decode video frame from packet
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
/*
    Use the attached code to get a feel for the functions to replace above call 
    https://www.ffmpeg.org/doxygen/4.0/decode__video_8c_source.html
*/


            // Did we get a video frame?
            if(frameFinished){
                // Convert the image from its native format(pCodecCtx->pix_fmt) to RGB
                sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
                            pFrame->linesize, 0, pCodecCtx->height,
                            pFrameRGB->data, pFrameRGB->linesize);
                
                // Save the frame to disk
                if(++i <= 5){
                    // writes RGB info to PPM file
                    SaveFrame(pFrameRGB, pCodecCtx->width,
                                pCodecCtx->height, i);
                }
            }
            // Free the packet that was allocated by av_read_frame
            av_packet_unref(&packet);
        }
    }

    // CLEAN UP
    // Free the RGB image
    av_free(buffer);
    av_free(pFrameRGB);
    // Free YUV frame
    av_free(pFrame);
    // Close codecs
    avcodec_close(pCodecCtx);
    avcodec_close(pCodecCtxOrig);
    // Close video file
    avformat_close_input(&pFormatCtx);

    return 0;
}

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame){
    FILE *pFile;
    char szFilename[32];
    int y;

    // Open file
    sprintf(szFilename, "frame%d.ppm", iFrame);
    pFile = fopen(szFilename, "wb");
    if(pFile == NULL)
        return;
    
    // Write header which indicates the width and height of image with max size of RGB values
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);

    // Write pixel data
    for(int y = 0; y < height; y++){
        fwrite(pFrame->data[0] + y * pFrame->linesize[0], 1, width * 3, pFile);
    }

    // Close file
    fclose(pFile);
}