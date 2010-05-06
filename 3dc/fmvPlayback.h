#ifndef _fmvPlayback_h_
#define _fmvPlayback_h_

#ifdef _XBOX
	#define _fseeki64 fseek // ensure libvorbis uses fseek and not _fseeki64 for xbox
#endif

#include "d3_func.h"
#include <fstream>
#include <map>
#include <string>
#include <ogg/ogg.h>
#include <theora/theora.h>
#include <theora/theoradec.h>
#include <vorbis/vorbisfile.h>
#include "ringbuffer.h"
#include "audioStreaming.h"

enum fmvErrors
{
	FMV_OK,
	FMV_INVALID_FILE,
	FMV_ERROR
};

enum streamType
{
	TYPE_VORBIS,
	TYPE_THEORA,
	TYPE_SKELETON,
	TYPE_UNKNOWN
};

// forward declare classes
class VorbisDecode;
class TheoraDecode;
class OggStream;

typedef std::map<int, OggStream*> streamMap;

class TheoraFMV
{
	public:
		std::string 	mFileName;
		std::ifstream 	mFileStream;

		// so we can easily reference these from the threads.
		OggStream *mVideo;
		OggStream *mAudio;

		ogg_sync_state 	mState;
		ogg_int64_t		mGranulePos;
		th_ycbcr_buffer mYuvBuffer;

		streamMap		mStreams;

		// audio
		StreamingAudioBuffer *mAudioStream;
		RingBuffer  *mRingBuffer;
		uint8_t		*mAudioData;
		uint16_t	*mAudioDataBuffer;
		uint32_t	mAudioDataBufferSize;

		// video
		RENDERTEXTURE mDisplayTexture;

		// test
		RENDERTEXTURE tex[3];
		uint32_t   texWidth[3];
		uint32_t   texHeight[3];

		uint32_t mTextureWidth;
		uint32_t mTextureHeight;
		uint32_t mFrameWidth;
		uint32_t mFrameHeight;
		CRITICAL_SECTION mFrameCriticalSection;
		bool mFrameCriticalSectionInited;

		// thread handles
		HANDLE mDecodeThreadHandle;
		HANDLE mAudioThreadHandle;

		bool mFmvPlaying;
		bool mFrameReady;
		bool mAudioStarted;

		// Offset of the page which was last read.
		ogg_int64_t mPageOffset;

		// Offset of first non-header page in file.
		ogg_int64_t mDataOffset;

		TheoraFMV() :
			mGranulePos(0),
			mAudioStream(0),
			mRingBuffer(0),
			mAudioData(0),
			mAudioDataBuffer(0),
			mAudioDataBufferSize(0),
			mDisplayTexture(0),
			mTextureWidth(0),
			mTextureHeight(0),
			mFrameWidth(0),
			mFrameHeight(0),
			mDecodeThreadHandle(0),
			mAudioThreadHandle(0),
			mVideo(0),
			mAudio(0),
			mFmvPlaying(false),
			mFrameReady(false),
			mAudioStarted(false),
			mPageOffset(0),
			mDataOffset(0),
			mFrameCriticalSectionInited(false)
		{

		}
		~TheoraFMV();

		int	Open(const std::string &fileName);
		void Close();
		ogg_int64_t TheoraFMV::ReadPage(ogg_page *page);
		bool ReadPacket(OggStream *stream, ogg_packet *packet);
		void ReadHeaders();
		void HandleTheoraData(OggStream *stream, ogg_packet *packet);
		bool IsPlaying();
		bool NextFrame();
		bool TheoraFMV::NextFrame(uint32_t width, uint32_t height, uint8_t *bufferPtr, uint32_t pitch);
		bool HandleTheoraHeader(OggStream* stream, ogg_packet* packet);
		bool HandleVorbisHeader(OggStream* stream, ogg_packet* packet);
		bool HandleSkeletonHeader(OggStream* stream, ogg_packet* packet);
};

#endif