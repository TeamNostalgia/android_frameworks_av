
#define LOG_NDEBUG 0
#define LOG_TAG "AmlogicPlayerRender"
#include "utils/Log.h"
#include <stdio.h>

#include "AmlogicPlayerRender.h"

#include <gui/Surface.h>
//#include <gui/ISurfaceTexture.h>
//#include <gui/SurfaceTextureClient.h>
#include <gui/ISurfaceComposer.h>

#include <android/native_window.h>
#include <cutils/properties.h>

#include "AmlogicPlayer.h"

#include <Amavutils.h>



#define  TRACE()	LOGV("[%s::%d]\n",__FUNCTION__,__LINE__)
///#define  TRACE()
#define OSDVIDEO_UPDATE_INTERVAL_MS 10
#define NORMAL_UPDATE_INTERVAL_MS 100
#define PAUSED_UPDATE_INTERVAL_MS 1000
namespace android {

AmlogicPlayerRender::AmlogicPlayerRender()
{
	AmlogicPlayerRender(NULL);
}

AmlogicPlayerRender::AmlogicPlayerRender(const sp<ANativeWindow> &nativeWindow,int flags)
	:Thread(false),mNativeWindow(nativeWindow)
{
	/*make sure first setting,*/
	mnewRect=Rect(0,0);
	moldRect=Rect(2,2);
	mcurRect=Rect(3,3);
	mVideoTransfer=0;
	mVideoTransferChanged=true;
	mWindowChanged=3;
	mVideoFrameReady=3;
	mUpdateInterval_ms=NORMAL_UPDATE_INTERVAL_MS;
	mRunning=false;
	mPaused=true;
	mOSDisScaled=false;
	nativeWidth = 640;
	nativeHeight = 400;
	amvideo_dev=NULL;
	mVideoScalingMode=NATIVE_WINDOW_SCALING_MODE_SCALE_TO_WINDOW;
	mFlags=flags;
	mLatestUpdateNum=0;
	return;
}


AmlogicPlayerRender::~AmlogicPlayerRender()
{
	Stop();
	updateOSDscaling(0);
	return;
}
status_t AmlogicPlayerRender::SwitchNativeWindow(const sp<ANativeWindow> &nativeWindow)
{
	Mutex::Autolock l(mMutex);
	mNativeWindow=nativeWindow;
	NativeWindowInit();
	return OK;
}
bool AmlogicPlayerRender::PlatformWantOSDscale(void)
{
	char mode[32]="panel";
	#define OSDSCALESET					"rw.fb.need2xscale"
	#define PLAYERER_ENABLE_SCALER		"media.amplayer.osd2xenable"
	#define DISP_MODE_PATH  				"/sys/class/display/mode"

	if(	AmlogicPlayer::PropIsEnable(PLAYERER_ENABLE_SCALER) && /*Player has enabled scaler*/
		AmlogicPlayer::PropIsEnable(OSDSCALESET) && /*Player framebuffer have enable*/
		(!amsysfs_get_sysfs_str(DISP_MODE_PATH,mode,32)&& !strncmp(mode,"1080p",5)))/*hdmi  1080p*/
	{
		LOGI("PlatformWantOSDscale true\n");

		return true;
	}

	return false;
}
status_t AmlogicPlayerRender::setVideoScalingMode(int mode)
{
	Mutex::Autolock l(mMutex);
	mVideoScalingMode = mode;
	LOGV("setVideoScalingMode %d\n",mode);
	if (mNativeWindow.get() != NULL) {
	status_t err = native_window_set_scaling_mode(
	        mNativeWindow.get(), mVideoScalingMode);
		if (err != OK) {
			ALOGW("Failed to set scaling mode: %d", err);
		}
		return err;
	}
	return UNKNOWN_ERROR;
}
int  AmlogicPlayerRender::updateOSDscaling(int enable)
{
	bool platneedscale;
	int needed=0;
	if(	mVideoTransfer==0 ||
		mVideoTransfer==HAL_TRANSFORM_ROT_180){
		needed=1;
		//only scale on equal or large than 720p
	}
	platneedscale=PlatformWantOSDscale();/*platform need it*/
	if(enable && needed && !mOSDisScaled && platneedscale){
		mOSDisScaled=true;
		LOGI("Enabled width scaling\n");
		amdisplay_utils_set_scale_mode(2,1);
	}else if((mOSDisScaled && !enable) || (mOSDisScaled && !needed)){
		LOGI("Disable width scaling\n");
		amdisplay_utils_set_scale_mode(1,1);
	}else{
		/*no changes do nothing*/
	}
	return 0;
}
//static
int  AmlogicPlayerRender::SwitchToOSDVideo(int enable)
{
    int ret;
    char newsetting[128];
    if(enable){
        amsysfs_set_sysfs_str("/sys/class/ppmgr/vtarget","1");
        amsysfs_set_sysfs_str("/sys/class/vfm/map","rm default");
	ret=amsysfs_set_sysfs_str("/sys/class/vfm/map","add default decoder ppmgr amlvideo");
	LOGI("enable osd video ...%d",ret);
    }else{
	char value[PROPERTY_VALUE_MAX];
        if(property_get("media.decoder.vfm.defmap", value, NULL)>0)
	{
	   LOGI("get def maping [%s]\n",value);
	}else{
	   strcpy(value,"decoder ppmgr amvideo ");
	}
	strcpy(newsetting,"add default ");
	strcat(newsetting,value);
        amsysfs_set_sysfs_str("/sys/class/ppmgr/vtarget","0");
        amsysfs_set_sysfs_str("/sys/class/vfm/map","rm default");
        ret=amsysfs_set_sysfs_str("/sys/class/vfm/map",newsetting);
	LOGI("disable osd video ...%d",ret);
    }

    return ret;
}
//static
int  AmlogicPlayerRender::SupportOSDVideo(void)
{
	int s1=	access("/sys/class/vfm/map",R_OK | W_OK);
	int s2=	access("/sys/class/ppmgr/vtarget",R_OK | W_OK);
	int s3=	access("/dev/video10",R_OK | W_OK);
	if(s1 || s2 || s3){
		LOGI("Kernel Not suport OSD video,access.map=%d,ppmgr/vtarget=%d,/dev/video10=%d\n",s1,s2,s3);
		return 0;
	}
	return 1;
}
status_t AmlogicPlayerRender::NativeWindowInit(void)
{
	if(!mNativeWindow.get())
		return UNKNOWN_ERROR;
	native_window_set_buffer_count(mNativeWindow.get(),3);
	if(amvideo_dev){
		native_window_set_scaling_mode(mNativeWindow.get(), mVideoScalingMode);
		native_window_set_buffers_geometry(mNativeWindow.get(), nativeWidth, nativeHeight, WINDOW_FORMAT_RGBA_8888);
		//HAL_PIXEL_FORMAT_YCrCb_420_SP);//WINDOW_FORMAT_RGBA_8888);
	}else{
		native_window_set_usage(mNativeWindow.get(),GRALLOC_USAGE_HW_TEXTURE | GRALLOC_USAGE_EXTERNAL_DISP | GRALLOC_USAGE_PRIVATE_0);
		native_window_set_buffers_format(mNativeWindow.get(),WINDOW_FORMAT_RGBA_8888);
		native_window_set_scaling_mode(mNativeWindow.get(), mVideoScalingMode);
	}
	return OK;
}
status_t AmlogicPlayerRender::OSDVideoInit(void)
{
	Mutex::Autolock l(mMutex);
	if(mFlags&1 &&
	   AmlogicPlayer::PropIsEnable("media.amplayer.v4osd.enable",true) &&
          amvideo_dev==NULL &&
          SupportOSDVideo())
	{
		int ret;
		int w,h;
		ret=amdisplay_utils_get_size(&w,&h);
		if(!ret){
			float zoomratio=AmlogicPlayer::PropGetFloat("media.amplayer.v4osd.zoom",0.6);
			if(zoomratio<0.1  || zoomratio>10){
				zoomratio=0.6;
			}
			nativeWidth=w*zoomratio;
			nativeHeight=h*zoomratio;
			if(nativeWidth!=w || nativeHeight!=h){
				/*force 64 bytes aligned.*/
				nativeWidth=(nativeWidth+63)&(~63);
				nativeHeight=(nativeHeight+63)&(~63);
			}
		}
		amvideo_dev=new_amvideo(FLAGS_V4L_MODE);
		LOGE("amvideo_init video size nativeWidth=%d,nativeHeight=%d\n",nativeWidth,nativeHeight);
		ret=amvideo_init(amvideo_dev,0,nativeWidth,nativeHeight,V4L2_PIX_FMT_RGB32);
		if(ret!=0){
			LOGE("amvideo_init failed =%d\n",ret);
			amvideo_release(amvideo_dev);
			amvideo_dev=NULL;
		}
	}
	return UNKNOWN_ERROR;
}
void AmlogicPlayerRender::onFirstRef()
{
	////if(mNativeWindow.get()!=NULL)
	{
		OSDVideoInit();
		NativeWindowInit();
	}
	return ;
}

status_t AmlogicPlayerRender::initCheck()
{
	return OK;
}

status_t AmlogicPlayerRender::readyToRun()
{
	 TRACE();

	return OK;
}


int  AmlogicPlayerRender::VideoFrameUpdate()
{
	vframebuf_t vf;
	char* vaddr;
	ANativeWindowBuffer* buf;
	Mutex::Autolock l(mMutex);
	int  vfvalid=0;
	int ret,ret1;
	if(mNativeWindow.get()==NULL){
		return 0;
	}
	if(amvideo_dev){
		ret = amlv4l_dequeuebuf(amvideo_dev,&vf);
		if (ret<0){
			return ret;
		}
		vfvalid=1;
	}
	int err=mNativeWindow->dequeueBuffer_DEPRECATED(mNativeWindow.get(), &buf);
	if (err != 0) {
		LOGE("dequeueBuffer failed: %s (%d)", strerror(-err), -err);
		amlv4l_queuebuf(amvideo_dev,&vf);
		return -1;
	}
	mNativeWindow->lockBuffer_DEPRECATED(mNativeWindow.get(), buf);
	sp<GraphicBuffer> graphicBuffer(new GraphicBuffer(buf, false));
	graphicBuffer->lock(1,(void **)&vaddr);
	if(vaddr!=NULL){
		if(vfvalid)
			memcpy(vaddr, vf.vbuf,nativeWidth*nativeHeight*4);
		else
			memset(vaddr,0x0,graphicBuffer->getWidth()*graphicBuffer->getHeight()*4);/*to show video in osd hole...*/
	}
	graphicBuffer->unlock();
	ret=mNativeWindow->queueBuffer_DEPRECATED(mNativeWindow.get(), buf);
	graphicBuffer.clear();
	if(vfvalid){
		ret1 =  amlv4l_queuebuf(amvideo_dev,&vf);
		if(ret != 0 || ret1!=0){
			LOGE("enqueue errno:%d", ret);
			return -1;
		}
		return 1;//refrashed one frame
	}
	return 0;
}


status_t AmlogicPlayerRender::Update()
{
	if(mNativeWindow.get()!=NULL &&(amvideo_dev|| mVideoFrameReady>0 || mWindowChanged>0 ) ){
		mVideoFrameReady--;
		mLatestUpdateNum=VideoFrameUpdate();
	}
	if(mWindowChanged){
		mWindowChanged--;
		updateOSDscaling(1);
	}
	return OK;
}
status_t AmlogicPlayerRender::ScheduleOnce()
{
	TRACE();
	mCondition.signal();
	return OK;
}


status_t AmlogicPlayerRender::Start()
{
	TRACE();
	Mutex::Autolock l(mMutex);
	if(!mRunning){
		if(amvideo_dev){
			SwitchToOSDVideo(1);
			amvideo_start(amvideo_dev);
		}
		mRunning=true;
		run("AmplayerRender");
	}
	if(amvideo_dev){
		mUpdateInterval_ms=OSDVIDEO_UPDATE_INTERVAL_MS;
	}else{
		mUpdateInterval_ms=NORMAL_UPDATE_INTERVAL_MS;
	}
	mPaused=false;
	ScheduleOnce();
	return OK;
}

status_t AmlogicPlayerRender::Stop()
{
	{
		Mutex::Autolock l(mMutex);
		mRunning=false;
	}
	requestExitAndWait();
	{
		Mutex::Autolock l(mMutex);
		if(amvideo_dev){
			amvideo_release(amvideo_dev);
			SwitchToOSDVideo(0);
			amvideo_dev=NULL;
		}
	}
	return OK;
}

status_t AmlogicPlayerRender::Pause()
{
	TRACE();
	Mutex::Autolock l(mMutex);
	ScheduleOnce();
	mPaused=true;
	mUpdateInterval_ms=PAUSED_UPDATE_INTERVAL_MS;
	mLatestUpdateNum=0;
	TRACE();
	return OK;
}



status_t AmlogicPlayerRender::onSizeChanged(Rect newR,Rect oldR)
{
	return OK;
}



bool AmlogicPlayerRender::threadLoop()
{
	{
		Mutex::Autolock l(mMutex);
		if(mLatestUpdateNum<=0)
			mCondition.waitRelative(mMutex,milliseconds_to_nanoseconds(mUpdateInterval_ms));
	}
	if(mRunning && !mPaused)
		Update();
	return true;
}

}
