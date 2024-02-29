#include <api/field_trials_view.h>
#include <output/webrtc_output.hpp>
#include <obs-output.h>
#include <obs.h>
#include <curl/curl.h>
#include <api/field_trials.h>
#include "api/video_codecs/video_decoder_factory.h"
#include "api/video_codecs/video_decoder_factory_template.h"
#include "api/video_codecs/video_decoder_factory_template_dav1d_adapter.h"
#include "api/video_codecs/video_decoder_factory_template_libvpx_vp8_adapter.h"
#include "api/video_codecs/video_decoder_factory_template_libvpx_vp9_adapter.h"
#include "api/video_codecs/video_decoder_factory_template_open_h264_adapter.h"
#include "api/video_codecs/video_encoder_factory.h"
#include "api/video_codecs/video_encoder_factory_template.h"
#include "api/video_codecs/video_encoder_factory_template_libaom_av1_adapter.h"
#include "api/video_codecs/video_encoder_factory_template_libvpx_vp8_adapter.h"
#include "api/video_codecs/video_encoder_factory_template_libvpx_vp9_adapter.h"
#include "api/video_codecs/video_encoder_factory_template_open_h264_adapter.h"
#include "api/audio/audio_mixer.h"
#include "api/audio_codecs/audio_decoder_factory.h"
#include "api/audio_codecs/audio_encoder_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/audio_options.h"
#include "api/create_peerconnection_factory.h"
#include "api/sequence_checker.h"
#include "system_wrappers/include/field_trial.h"
void register_webrtc_output()
{
	struct obs_output_info info {};
	info.id = "webrtc_output";
	info.flags = OBS_OUTPUT_VIDEO | OBS_OUTPUT_SERVICE;
	info.get_name = [](void *) -> const char * {
		return obs_module_text("Output.name");
	};
	info.create = [](obs_data_t *settings, obs_output_t *output) -> void * {
		return rtc::make_ref_counted<RTC::WHIPOutput>(settings, output)
			.release();
	};
	info.destroy = [](void *p) {
		delete static_cast<RTC::WHIPOutput *>(p);
	};
	info.start = [](void *p) -> bool {
		return static_cast<RTC::WHIPOutput *>(p)->Start();
	};
	info.stop = [](void *, uint64_t) {
		//TODO: STOP
	};
	info.raw_audio = [](void *p, struct audio_data *) {
		auto output = static_cast<RTC::WHIPOutput *>(p);
		(void)output;
	};
	info.raw_video = [](void *p, struct video_data *frame) {
		// RTC_LOG(LS_INFO) << "raw_video";
		static_cast<RTC::WHIPOutput *>(p)->OnFrame(frame);
	};
	info.protocols = "WHIP";
	info.get_defaults = [](obs_data_t *) {
	};
	info.get_properties = [](void *) -> obs_properties_t * {
		return obs_properties_create();
	};

	info.get_connect_time_ms = [](void *) -> int {
		return 100;
	};
	info.encoded_packet = [](void *, struct encoder_packet *packet) {
		(void)packet;
	};
	info.encoded_video_codecs = "h264";
	info.encoded_audio_codecs = "opus";
	webrtc::field_trial::InitFieldTrialsFromString(
		"WebRTC-FlexFEC-03-Advertised/Enabled/WebRTC-FlexFEC-03/Enabled/WebRTC-DisableUlpFecExperiment/Enabled/");
	obs_register_output(&info);
}

namespace RTC {

WHIPOutput::WHIPOutput(obs_data_t *, obs_output_t *output)
	: signal_thread_(rtc::Thread::CreateWithSocketServer()),
	  output_(output)
{
	video_capture_ = std::make_unique<OBSVideoCapture>();
	video_track_source_ =
		rtc::make_ref_counted<OBSTrackSource>(video_capture_.get());
	signal_thread_->SetName("signal_thread", nullptr);
	RTC_CHECK(signal_thread_->Start());

	pc_factory_ = webrtc::CreatePeerConnectionFactory(
		nullptr /* network_thread */, nullptr /* worker_thread */,
		signal_thread_.get(), nullptr /* default_adm */,
		webrtc::CreateBuiltinAudioEncoderFactory(),
		webrtc::CreateBuiltinAudioDecoderFactory(),
		std::make_unique<webrtc::VideoEncoderFactoryTemplate<
			// webrtc::LibvpxVp8EncoderTemplateAdapter,
			// webrtc::LibvpxVp9EncoderTemplateAdapter,
			webrtc::OpenH264EncoderTemplateAdapter
			// webrtc::LibaomAv1EncoderTemplateAdapter>>(),
			>>(),
		std::make_unique<webrtc::VideoDecoderFactoryTemplate<
			// webrtc::LibvpxVp8DecoderTemplateAdapter,
			// webrtc::LibvpxVp9DecoderTemplateAdapter,
			webrtc::OpenH264DecoderTemplateAdapter>>(),
		// webrtc::Dav1dDecoderTemplateAdapter>>(),
		nullptr /* audio_mixer */, nullptr /* audio_processing */);
}

bool WHIPOutput::Start()
{
	if (!obs_output_can_begin_data_capture(output_, 0))
		return false;

	bool flag = start_.load(std::memory_order_acquire);
	do {
		if (flag)
			return true;
	} while (!start_.compare_exchange_weak(flag, true,
					       std::memory_order_acq_rel));

	signal_thread_->PostTask([this]() {
		obs_service_t *service = obs_output_get_service(output_);

		if (!service) {
			obs_output_signal_stop(output_, OBS_OUTPUT_ERROR);
			return;
		}

		url_ = obs_service_get_connect_info(
			service, OBS_SERVICE_CONNECT_INFO_SERVER_URL);

		if (url_.empty()) {
			obs_output_signal_stop(output_, OBS_OUTPUT_BAD_PATH);
			return;
		}
		rtc::LogMessage::LogToDebug(rtc::LoggingSeverity::LS_VERBOSE);

		token_ = obs_service_get_connect_info(
			service, OBS_SERVICE_CONNECT_INFO_BEARER_TOKEN);

		webrtc::PeerConnectionInterface::RTCConfiguration config;
		// config.type = webrtc::PeerConnectionInterface::kRelay;
		config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
		webrtc::PeerConnectionDependencies pc_dependencies(this);
		// pc_dependencies.trials.reset(new webrtc::FieldTrials(
		// 	"WebRTC-FlexFEC-03-Advertised/Enabled/WebRTC-FlexFEC-03/Enabled/WebRTC-DisableUlpFecExperiment/Enabled/"));
		RTC_LOG(LS_INFO) << "fields : "
				 << webrtc::field_trial::GetFieldTrialString();
		auto res = pc_factory_->CreatePeerConnectionOrError(
			config, std::move(pc_dependencies));
		RTC_CHECK(res.ok());
		pc_ = std::move(res.value());
		AddTracks();
		pc_->CreateOffer(
			this,
			webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
	});

	return true;
}

void WHIPOutput::AddTracks()
{
	video_track_ = pc_factory_->CreateVideoTrack(video_track_source_,
						     "video_label");
	RTC_CHECK(video_track_ != nullptr) << "create video track failed!";

	auto result = pc_->AddTrack(video_track_, {"stream"});
	RTC_CHECK(result.ok()) << "failed to add video track to pc: "
			       << result.error().message();

	rtc::scoped_refptr<webrtc::AudioTrackInterface> audio_track(
		pc_factory_->CreateAudioTrack(
			"audio_label",
			pc_factory_->CreateAudioSource(cricket::AudioOptions())
				.get()));

	result = pc_->AddTrack(audio_track, {"stream"});
	RTC_CHECK(result.ok()) << "failed to add audio track to pc: "
			       << result.error().message();
}

class DummySetSessionDescriptionObserver
	: public webrtc::SetSessionDescriptionObserver {
public:
	static rtc::scoped_refptr<DummySetSessionDescriptionObserver> Create()
	{
		return rtc::make_ref_counted<
			DummySetSessionDescriptionObserver>();
	}
	virtual void OnSuccess() { RTC_LOG(LS_INFO) << __FUNCTION__; }
	virtual void OnFailure(webrtc::RTCError error)
	{
		RTC_LOG(LS_INFO)
			<< __FUNCTION__ << " " << ToString(error.type()) << ": "
			<< error.message();
	}
};

void WHIPOutput::OnSuccess(webrtc::SessionDescriptionInterface *desc)
{
	pc_->SetLocalDescription(
		DummySetSessionDescriptionObserver::Create().get(), desc);
	std::string sdp;
	desc->ToString(&sdp);
	RTC_LOG(LS_INFO) << "local sdp: " << sdp;

	if (ConnectServer(sdp)) {
		RTC_CHECK(obs_output_begin_data_capture(output_, 0))
			<< "obs capture failed";
	}
}

void WHIPOutput::OnFailure(webrtc::RTCError error)
{
	RTC_LOG(LS_INFO) << __FUNCTION__ << " " << ToString(error.type())
			 << ": " << error.message();
}

static size_t curl_writefunction(char *data, size_t size, size_t nmemb,
				 void *priv_data)
{
	auto read_buffer = static_cast<std::string *>(priv_data);

	size_t real_size = size * nmemb;

	read_buffer->append(data, real_size);
	return real_size;
}

static std::string trim_string(const std::string &source)
{
	std::string ret(source);
	ret.erase(0, ret.find_first_not_of(" \n\r\t"));
	ret.erase(ret.find_last_not_of(" \n\r\t") + 1);
	return ret;
}

#define LOCATION_HEADER_LENGTH 10

static size_t curl_header_location_function(char *data, size_t size,
					    size_t nmemb, void *priv_data)
{
	auto header_buffer = static_cast<std::vector<std::string> *>(priv_data);

	size_t real_size = size * nmemb;

	if (real_size < LOCATION_HEADER_LENGTH)
		return real_size;

	if (!strncasecmp(data, "location: ", LOCATION_HEADER_LENGTH)) {
		char *val = data + LOCATION_HEADER_LENGTH;
		auto header_temp =
			std::string(val, real_size - LOCATION_HEADER_LENGTH);

		header_temp = trim_string(header_temp);
		header_buffer->push_back(header_temp);
	}

	return real_size;
}

bool WHIPOutput::ConnectServer(const std::string &sdp)
{
	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Content-Type: application/sdp");
	if (!token_.empty()) {
		auto bearer_token_header =
			std::string("Authorization: Bearer ") + token_;
		headers =
			curl_slist_append(headers, bearer_token_header.c_str());
	}

	std::string read_buffer;
	std::vector<std::string> location_headers;

	CURL *c = curl_easy_init();
	curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, curl_writefunction);
	curl_easy_setopt(c, CURLOPT_WRITEDATA, (void *)&read_buffer);
	curl_easy_setopt(c, CURLOPT_HEADERFUNCTION,
			 curl_header_location_function);
	curl_easy_setopt(c, CURLOPT_HEADERDATA, (void *)&location_headers);
	curl_easy_setopt(c, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(c, CURLOPT_URL, url_.c_str());
	curl_easy_setopt(c, CURLOPT_POST, 1L);
	curl_easy_setopt(c, CURLOPT_COPYPOSTFIELDS, sdp.c_str());
	curl_easy_setopt(c, CURLOPT_TIMEOUT, 8L);
	curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(c, CURLOPT_UNRESTRICTED_AUTH, 1L);

	std::shared_ptr<int> clean_up(new int(1), [c, headers](int *p) {
		delete p;
		curl_easy_cleanup(c);
		curl_slist_free_all(headers);
	});

	CURLcode res = curl_easy_perform(c);
	if (res != CURLE_OK) {
		RTC_LOG(LS_INFO)
			<< "Connect failed: CURL returned result not CURLE_OK";
		obs_output_signal_stop(output_, OBS_OUTPUT_CONNECT_FAILED);
		return false;
	}

	long response_code;
	curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &response_code);
	if (response_code != 201) {
		RTC_LOG(LS_INFO)
			<< "Connect failed: HTTP endpoint returned response code"
			<< response_code;
		obs_output_signal_stop(output_, OBS_OUTPUT_INVALID_STREAM);
		return false;
	}

	if (read_buffer.empty()) {
		RTC_LOG(LS_INFO)
			<< "Connect failed: No data returned from HTTP endpoint request";
		obs_output_signal_stop(output_, OBS_OUTPUT_CONNECT_FAILED);
		return false;
	}

	long redirect_count = 0;
	curl_easy_getinfo(c, CURLINFO_REDIRECT_COUNT, &redirect_count);

	if (location_headers.size() < static_cast<size_t>(redirect_count) + 1) {
		RTC_LOG(LS_INFO)
			<< "WHIP server did not provide a resource URL via the Location header";
		obs_output_signal_stop(output_, OBS_OUTPUT_CONNECT_FAILED);
		return false;
	}

	CURLU *url_builder = curl_url();
	auto last_location_header = location_headers.back();

	// If Location header doesn't start with `http` it is a relative URL.
	// Construct a absolute URL using the host of the effective URL
	if (last_location_header.find("http") != 0) {
		char *effective_url = nullptr;
		curl_easy_getinfo(c, CURLINFO_EFFECTIVE_URL, &effective_url);
		if (effective_url == nullptr) {
			RTC_LOG(LS_INFO) << "failed to build resource URL";
			obs_output_signal_stop(output_,
					       OBS_OUTPUT_CONNECT_FAILED);
			return false;
		}

		curl_url_set(url_builder, CURLUPART_URL, effective_url, 0);
		curl_url_set(url_builder, CURLUPART_PATH,
			     last_location_header.c_str(), 0);
		curl_url_set(url_builder, CURLUPART_QUERY, "", 0);
	} else {
		curl_url_set(url_builder, CURLUPART_URL,
			     last_location_header.c_str(), 0);
	}

	char *url = nullptr;
	CURLUcode rc = curl_url_get(url_builder, CURLUPART_URL, &url,
				    CURLU_NO_DEFAULT_PORT);
	if (rc) {
		RTC_LOG(LS_INFO)
			<< "WHIP server provided a invalid resource URL via the Location header";
		obs_output_signal_stop(output_, OBS_OUTPUT_CONNECT_FAILED);
		return false;
	}

	webrtc::SdpParseError error;
	auto remote_sdp = webrtc::CreateSessionDescription(
		webrtc::SdpType::kAnswer, read_buffer, &error);

	if (!remote_sdp) {
		RTC_LOG(LS_ERROR)
			<< "Can't parse received session description message. "
			   "SdpParseError was: "
			<< error.description;
		obs_output_signal_stop(output_, OBS_OUTPUT_CONNECT_FAILED);
		return false;
	}

	RTC_LOG(LS_INFO) << "remote sdp: " << read_buffer;

	pc_->SetRemoteDescription(
		DummySetSessionDescriptionObserver::Create().get(),
		remote_sdp.release());

	return true;
}

void WHIPOutput::OnIceConnectionChange(
	webrtc::PeerConnectionInterface::IceConnectionState state)
{
	RTC_LOG(LS_INFO) << "IceState: "
			 << webrtc::PeerConnectionInterface::AsString(state);
}

void WHIPOutput::OnIceGatheringChange(
	webrtc::PeerConnectionInterface::IceGatheringState state)
{
	RTC_LOG(LS_INFO) << "IceGatheringState: "
			 << webrtc::PeerConnectionInterface::AsString(state);
}

void WHIPOutput::OnIceCandidate(const webrtc::IceCandidateInterface *candidate)
{
	std::string str;
	candidate->ToString(&str);
	RTC_LOG(LS_INFO) << "candidate: " << str;
}

void WHIPOutput::OnFrame(video_data *frame)
{
	video_capture_->OnFrame(frame);
}

} // namespace RTC
