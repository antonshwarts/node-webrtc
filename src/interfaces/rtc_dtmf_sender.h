/* Copyright (c) 2019 The node-webrtc project authors. All rights reserved.
 *
 * Use of this source code is governed by a BSD-style license that can be found
 * in the LICENSE.md file in the root of the source tree. All contributing
 * project authors may be found in the AUTHORS file in the root of the source
 * tree.
 */
#pragma once

#include <node-addon-api/napi.h>
#include <webrtc/api/dtmf_sender_interface.h>
#include <webrtc/api/scoped_refptr.h>

#include "src/node/async_object_wrap_with_loop.h"
#include "src/node/wrap.h"

namespace webrtc { class RTCError; }

namespace node_webrtc {

class PeerConnectionFactory;

class RTCDTMFSender
  : public AsyncObjectWrapWithLoop<RTCDTMFSender>
  , public webrtc::DtmfSenderObserverInterface {
 public:
  static const int DtmfDefaultDurationMs = 100;
  static const int DtmfDefaultGapMs = 50;

  explicit RTCDTMFSender(const Napi::CallbackInfo&);

  ~RTCDTMFSender() override;

  static void Init(Napi::Env, Napi::Object);

  void OnToneChange(const std::string& tone,
                    const std::string& tone_buffer) override;

  static ::node_webrtc::Wrap <
  RTCDTMFSender*,
  rtc::scoped_refptr<webrtc::DtmfSenderInterface>,
  PeerConnectionFactory*
  > * wrap();

 protected:
  void Stop() override;

 private:
  static Napi::FunctionReference& constructor();

  Napi::Value CanInsertDtmf(const Napi::CallbackInfo&);
  Napi::Value ToneBuffer(const Napi::CallbackInfo&);
  Napi::Value InsertDtmf(const Napi::CallbackInfo&);

  Napi::Value JsStop(const Napi::CallbackInfo&);

  static RTCDTMFSender* Create(
      PeerConnectionFactory*,
      rtc::scoped_refptr<webrtc::DtmfSenderInterface>);

  PeerConnectionFactory* _factory;
  rtc::scoped_refptr<webrtc::DtmfSenderInterface> _dtmf;
};

}  // namespace node_webrtc
