
#ifndef ANDROID_AMLOGIC_PLAYER_RENDER_H
#define ANDROID_AMLOGIC_PLAYER_RENDER_H

#include <stdint.h>
#include <sys/types.h>
#include <limits.h>

#include <utils/Atomic.h>
#include <utils/Errors.h>
#include <utils/threads.h>
#include <android/native_window.h>
#include <ui/Rect.h>

extern "C" {
#include <amvideo.h>
}

namespace android {

class AmlogicPlayerRender: public Thread {

	public:
		AmlogicPlayerRender(void);
		AmlogicPlayerRender(const sp<ANativeWindow> &nativeWindow,int flags=0);
		~AmlogicPlayerRender();
		virtual status_t	readyToRun();
		virtual	void 		onFirstRef();
		virtual status_t	Start();
		virtual status_t	Stop();
		virtual status_t	Pause();

		virtual status_t 	onSizeChanged(Rect newR,Rect oldR);
				int 		updateOSDscaling(int enable);

		static  bool 		PlatformWantOSDscale(void);
		static  int             SwitchToOSDVideo(int enable);
		static  int             SupportOSDVideo(void);
		status_t 			setVideoScalingMode(int);
		status_t 			SwitchNativeWindow(const sp<ANativeWindow> &nativeWindow);
	private:
		status_t	NativeWindowInit(void);
		status_t	OSDVideoInit(void);
		bool		threadLoop();
		status_t	Update();
		int 	VideoFrameUpdate();
		status_t	VideoPositionUpdate();
		status_t	ScheduleOnce();
		status_t	initCheck();


		sp<ANativeWindow> 	mNativeWindow;

		int 		mUpdateInterval_ms;

		Rect 		mnewRect;
		Rect 		moldRect;
		Rect 		mcurRect;
		int 		mVideoTransfer;
		bool 		mVideoTransferChanged;
		int			mWindowChanged;
		int			mVideoFrameReady;

		Mutex   	mMutex;
		Condition	mCondition;

		bool 		mRunning;
		bool 		mPaused;

		bool 		mOSDisScaled;
		int 			mVideoScalingMode;

		amvideo_dev_t *amvideo_dev;

		int nativeWidth;
		int nativeHeight;
		int mFlags; //bit0:osdvideo..
		int mLatestUpdateNum;
};

}
#endif
