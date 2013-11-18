
#ifndef AMLOGICPLAYEREXTRACTORDEMUX__HH
#define AMLOGICPLAYEREXTRACTORDEMUX__HH


#ifdef __cplusplus
extern "C" {
#include "libavutil/avstring.h"
#include "libavformat/avformat.h"
}
#endif
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MediaExtractor.h>
#include"AmlogicPlayerExtractorDataSource.h"
#include <media/MediaPlayerInterface.h>
#include <media/AudioTrack.h>
#include <WVMExtractor.h>
namespace android
{

#include <sys/types.h>
#include <utils/Errors.h>
#include <utils/KeyedVector.h>
#include <utils/String8.h>
#include <utils/RefBase.h>
#include <utils/threads.h>
class AmlogicPlayerExtractorDemux : public RefBase
{

public:
    AmlogicPlayerExtractorDemux(AVFormatContext *s);
    ~AmlogicPlayerExtractorDemux();

public:
    //static
    static int BasicInit(void);
    static int RegisterExtractor(void);

private:
    //private static for demux struct.
    static int extractor_probe(AVProbeData *p)  ;
    static int extractor_read_header(AVFormatContext *s, AVFormatParameters *ap);
    static int extractor_read_packet(AVFormatContext *s, AVPacket *pkt);
    static int extractor_close(AVFormatContext *s);
    static int extractor_read_seek(AVFormatContext *s, int stream_index, int64_t timestamp, int flags);
private:
    int Probe(AVProbeData *p)   ;
    int ReadHeader(AVFormatContext *s, AVFormatParameters *ap);
    int ReadPacket(AVFormatContext *s, AVPacket *pkt);
    int Close(AVFormatContext *s);
    int ReadSeek(AVFormatContext *s, int stream_index, int64_t timestamp, int flags);
    int SetStreamInfo(AVStream *st, sp<MetaData> meta,  String8 mime);

   int8_t mVideoIndex;
   int8_t mAudioIndex;
   bool hasVideo;
   bool hasAudio;
   bool IsVideoAlreadyStarted;
   bool IsAudioAlreadyStarted;
    int64_t mLastVideoTimeUs;
    int64_t mLastAudioTimeUs;
    sp<DataSource> mReadDataSouce;
    sp<WVMExtractor> mWVMExtractor;
    sp<MediaExtractor> mMediaExtractor;
    //sp<MediaSource> mTrack;
   sp<MediaSource>mVideoTrack;
   sp<MediaSource>mAudioTrack;
   MediaBuffer *mBuffer;
   char *smimeType;
   float confidence;
    enum SeekType {
        NO_SEEK,
        SEEK,
        SEEK_VIDEO_ONLY
    };
    SeekType mSeeking;
     static  AVInputFormat DrmDemux;
     int64_t mSeekTimestamp;
     //static DrmManagerClient *mDrmManagerClient;
    // static sp<DecryptHandle> mDecryptHandle;
};



};//namespace android
#endif
