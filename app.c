#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

// Downloaded ffmpeg source file and configured it using ./configure from git bash, later adding those folders to mingw include directory manually
// Refer to https://trac.ffmpeg.org/wiki/CompilationGuide/MinGW

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
    // Copy context
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if(avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0){
        fprintf(stderr, "Couldn't copy codec context");
        return -1; // Error copying codec context
    }

    // Open codec
    if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
    return -1; // Could not open Codec

    

}
