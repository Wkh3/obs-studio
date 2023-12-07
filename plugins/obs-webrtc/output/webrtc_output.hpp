#pragma once
#include <obs.h>
#include <obs-module.h>
#include <api/video/video_source_interface.h>
#include <api/video/video_frame.h>
#include <media/base/video_adapter.h>
#include <media/base/video_broadcaster.h>
#include <api/peer_connection_interface.h>
#include <output/video_source.hpp>
namespace RTC{
class WHIPOutput : public webrtc::PeerConnectionObserver,
                   public webrtc::CreateSessionDescriptionObserver
{
public:
    WHIPOutput(obs_data_t *settings,obs_output_t *output);

    virtual ~WHIPOutput(){}

    bool Start();

    void OnFrame(video_data * frame);
public:
    //implement PeerConnectionObserver
    void OnSignalingChange(
      webrtc::PeerConnectionInterface::SignalingState ) override {}
    
    void OnAddTrack(
        rtc::scoped_refptr<webrtc::RtpReceiverInterface> ,
        const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&
            ) override {}
    
    void OnRemoveTrack(
        rtc::scoped_refptr<webrtc::RtpReceiverInterface> ) override {}
    
    void OnDataChannel(
        rtc::scoped_refptr<webrtc::DataChannelInterface> ) override {}
    
    void OnRenegotiationNeeded() override {}
    
    void OnIceConnectionChange(
        webrtc::PeerConnectionInterface::IceConnectionState ) override ;
    
    void OnIceGatheringChange(
        webrtc::PeerConnectionInterface::IceGatheringState ) override ;
    
    void OnIceCandidate(const webrtc::IceCandidateInterface* ) override;

    void OnIceConnectionReceivingChange(bool ) override {}
public:
    //implement CreateSessionDescriptionObserver
    void OnSuccess(webrtc::SessionDescriptionInterface*) override;

    void OnFailure(webrtc::RTCError) override;

private:
    bool ConnectServer(const std::string &);
    void AddTracks();
private:
    obs_output_t *output_;
    std::string url_;
    std::string token_;
    std::unique_ptr<OBSVideoCapture> video_capture_;
    rtc::scoped_refptr<OBSTrackSource> video_track_source_; 
    rtc::scoped_refptr<webrtc::VideoTrackInterface> video_track_;
    std::atomic_bool start_{false};
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> pc_;
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> pc_factory_;
    std::unique_ptr<rtc::Thread> signal_thread_;
};

}

void register_webrtc_output();
