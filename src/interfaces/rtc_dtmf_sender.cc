/* Copyright (c) 2019 The node-webrtc project authors. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be found
 * in the LICENSE.md file in the root of the source tree. All contributing
 * project authors may be found in the AUTHORS file in the root of the source
 * tree.
 */
#include "src/interfaces/rtc_dtmf_sender.h"

#include <array>

#include <node-addon-api/napi.h>
#include <webrtc/rtc_base/location.h>
#include <webrtc/rtc_base/thread.h>

#include "src/converters.h"
#include "src/converters/arguments.h"
#include "src/converters/napi.h"
#include "src/converters/webrtc.h"
#include "src/dictionaries/macros/napi.h"
#include "src/functional/maybe.h"
#include "src/interfaces/rtc_peer_connection/peer_connection_factory.h"
#include "src/node/events.h"

namespace node_webrtc {

Napi::FunctionReference& RTCDTMFSender::constructor() {
  static Napi::FunctionReference constructor;
  return constructor;
}

RTCDTMFSender::RTCDTMFSender(const Napi::CallbackInfo& info)
  : AsyncObjectWrapWithLoop<RTCDTMFSender>("RTCDTMFSender", *this, info) {
  if (info.Length() != 2 || !info[0].IsObject() || !info[1].IsExternal()) {
    Napi::TypeError::New(info.Env(), "You cannot construct an RTCDTMFSender").ThrowAsJavaScriptException();
    return;
  }

  auto factory = PeerConnectionFactory::Unwrap(info[0].ToObject());
  auto dtmf = *info[1].As<Napi::External<rtc::scoped_refptr<webrtc::DtmfSenderInterface>>>().Data();

  _factory = factory;
  _factory->Ref();

  _dtmf = std::move(dtmf);
  _dtmf->RegisterObserver(this);
}

RTCDTMFSender::~RTCDTMFSender() {
  Napi::HandleScope scope(PeerConnectionFactory::constructor().Env());
  _factory->Unref();
  _factory = nullptr;
  wrap()->Release(this);
}  // NOLINT

void RTCDTMFSender::Stop() {
  _dtmf->UnregisterObserver();
  AsyncObjectWrapWithLoop<RTCDTMFSender>::Stop();
}


void RTCDTMFSender::OnToneChange(const std::string& tone, const std::string& tone_buffer) {
  (void) tone_buffer;
  Dispatch(CreateCallback<RTCDTMFSender>([this, tone, tone_buffer]() {
    auto env = Env();
    Napi::HandleScope scope(env);
    auto event = Napi::Object::New(env);
    event.Set("tone", Napi::String::New(env, tone));
    MakeCallback("ontonechange", { event });
  }));
}

Napi::Value RTCDTMFSender::CanInsertDtmf(const Napi::CallbackInfo& info) {
  CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), _dtmf->CanInsertDtmf(), result, Napi::Value)
  return result;
}

Napi::Value RTCDTMFSender::ToneBuffer(const Napi::CallbackInfo& info) {
  CONVERT_OR_THROW_AND_RETURN_NAPI(info.Env(), _dtmf->tones(), result, Napi::Value)
  return result;
}

Napi::Value RTCDTMFSender::InsertDtmf(const Napi::CallbackInfo& info) {
  auto maybeTones = From<std::string>(info[0]);
  if (maybeTones.IsInvalid()) {
    auto error = maybeTones.ToErrors()[0];
    Napi::TypeError::New(info.Env(), error).ThrowAsJavaScriptException();
    return info.Env().Undefined();
  }

  std::array<int, 2> numbers = {DtmfDefaultDurationMs, DtmfDefaultGapMs};
  const auto length = std::min(numbers.size(), info.Length());
  for (size_t i = 1; i < length; i++) {
    auto maybeNumber = From<int>(info[i]);
    if (maybeNumber.IsValid()) {
      numbers[i - 1] = maybeNumber.UnsafeFromValid();
    }
  }

  auto tones = maybeTones.UnsafeFromValid();
  _dtmf->InsertDtmf(tones, numbers[0], numbers[1]);
  return info.Env().Undefined();
}

Napi::Value RTCDTMFSender::JsStop(const Napi::CallbackInfo& info) {
  Stop();
  return info.Env().Undefined();
}

Wrap <
RTCDTMFSender*,
rtc::scoped_refptr<webrtc::DtmfSenderInterface>,
PeerConnectionFactory*
> * RTCDTMFSender::wrap() {
  static auto wrap = new node_webrtc::Wrap <
  RTCDTMFSender*,
  rtc::scoped_refptr<webrtc::DtmfSenderInterface>,
  PeerConnectionFactory*
  > (RTCDTMFSender::Create);
  return wrap;
}

RTCDTMFSender* RTCDTMFSender::Create(
    PeerConnectionFactory* factory,
    rtc::scoped_refptr<webrtc::DtmfSenderInterface> dtmf) {
  auto env = constructor().Env();
  Napi::HandleScope scope(env);

  auto object = constructor().New({
    factory->Value(),
    Napi::External<rtc::scoped_refptr<webrtc::DtmfSenderInterface>>::New(env, &dtmf)
  });

  return RTCDTMFSender::Unwrap(object);
}

void RTCDTMFSender::Init(Napi::Env env, Napi::Object exports) {
  auto func = DefineClass(env, "RTCDTMFSender", {
    InstanceAccessor("canInsertDTMF", &RTCDTMFSender::CanInsertDtmf, nullptr),
    InstanceAccessor("toneBuffer", &RTCDTMFSender::ToneBuffer, nullptr),
    InstanceMethod("insertDTMF", &RTCDTMFSender::InsertDtmf),
    InstanceMethod("stop", &RTCDTMFSender::JsStop)
  });

  constructor() = Napi::Persistent(func);
  constructor().SuppressDestruct();

  exports.Set("RTCDTMFSender", func);
}

}  // namespace node_webrtc
