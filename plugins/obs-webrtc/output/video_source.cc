#include <output/video_source.hpp>
#include <obs.h>
#include <api/video/nv12_buffer.h>
#include <api/video/video_rotation.h>
namespace RTC{
void OBSVideoCapture::AddOrUpdateSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink,
                               const rtc::VideoSinkWants& wants){
    broadcaster_.AddOrUpdateSink(sink,wants);
    UpdateVideoAdapter();
}
  
void OBSVideoCapture::RemoveSink(rtc::VideoSinkInterface<webrtc::VideoFrame>* sink) {
    broadcaster_.RemoveSink(sink);
    UpdateVideoAdapter();
}

void OBSVideoCapture::OnFrame(video_data *frame){
    // RTC_LOG(LS_INFO) << "input frame ts :" << frame->timestamp;
    obs_video_info ovi;
    
    if(!obs_get_video_info(&ovi))
       return;

    rtc::scoped_refptr<webrtc::VideoFrameBuffer> buffer = nullptr;

    switch(ovi.output_format){
    case VIDEO_FORMAT_NV12: {
        auto nv12_buffer = webrtc::NV12Buffer::Create(ovi.output_width,ovi.output_height,frame->linesize[0],frame->linesize[1]);

        std::copy(frame->data[0],frame->data[0] + frame->linesize[0] * ovi.output_height
                  ,nv12_buffer->MutableDataY());
        
        std::copy(frame->data[1],frame->data[1] + frame->linesize[1] * ((ovi.output_height + 1) / 2)
                  ,nv12_buffer->MutableDataUV());
        buffer = nv12_buffer->ToI420();
        break;
    }
    default:
        return;
    }
    // webrtc::NV12Buffer
    //TODO: adapter video frame
    auto video_frame = webrtc::VideoFrame::Builder().set_video_frame_buffer(buffer)
                                 .set_timestamp_us(rtc::TimeMicros())
                                 .set_rotation(webrtc::kVideoRotation_0)
                                 .build();

    broadcaster_.OnFrame(video_frame);
}

void OBSVideoCapture::UpdateVideoAdapter(){
    video_adapter_.OnSinkWants(broadcaster_.wants());
}

}
