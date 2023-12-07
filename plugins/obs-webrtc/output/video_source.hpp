#pragma once
#include <api/video/video_source_interface.h>
#include <pc/video_track_source.h>
#include <media/base/video_adapter.h>
#include <media/base/video_broadcaster.h>
#include <media-io/video-io.h>
namespace RTC{

class OBSVideoCapture : public rtc::VideoSourceInterface<webrtc::VideoFrame> {
public:
  void AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                               const rtc::VideoSinkWants& wants) override;
  
  void RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) override;

  void OnFrame(video_data *frame);
private:
  void UpdateVideoAdapter();
private:
    rtc::VideoBroadcaster broadcaster_;
    cricket::VideoAdapter video_adapter_;
};


class OBSTrackSource : public webrtc::VideoTrackSource
{
public:
  OBSTrackSource(OBSVideoCapture *capture) : webrtc::VideoTrackSource(false),
                                        capture_(capture){
  }
private:
  rtc::VideoSourceInterface<webrtc::VideoFrame>* source() override {
    return capture_;
  }
private:
  OBSVideoCapture* capture_;
};

}
