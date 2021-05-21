'use strict';

const tape = require('tape');
const RTCAudioSource = require('..').nonstandard.RTCAudioSource;
const RTCVideoSource = require('..').nonstandard.RTCVideoSource;
const { negotiateRTCPeerConnections } = require('./lib/pc');

tape('RTCDTMFSender', async t => {
  const audioSource = new RTCAudioSource();
  const audioTrack = audioSource.createTrack();
  const videoSource = new RTCVideoSource();
  const videoTrack = videoSource.createTrack();

  const [pc1, pc2] = await negotiateRTCPeerConnections({
    withPc1(pc1) {
      pc1.addTrack(audioTrack);
      pc1.addTrack(videoTrack);
    }
  });
  t.equal(pc1.getSenders().length, 2, 'getSenders returns a non-empty Array');

  const audioSender = pc1.getSenders()[0];
  t.ok(audioSender.dtmf, 'audio sender with .dtmf');
  const videoSender = pc1.getSenders()[1];
  t.notOk(videoSender.dtmf, 'video sender without .dtmf');

  t.equal(audioSender.dtmf.canInsertDTMF, true, 'canInsertDTMF for audio is true');

  const tones = [];
  const onToneChangePromise = new Promise(resolve => {
    audioSender.dtmf.ontonechange = (event) => {
      if (event.tone !== '') {
        tones.push(event.tone);
      } else {
        resolve();
      }
    };
  });

  t.equal(audioSender.dtmf.toneBuffer.length, 0, 'toneBuffer is empty');
  audioSender.dtmf.insertDTMF('12345');
  t.notEqual(audioSender.dtmf.toneBuffer.length, 0, 'toneBuffer is not empty');
  await onToneChangePromise;
  t.equal(audioSender.dtmf.toneBuffer.length, 0, 'sent: toneBuffer is empty');
  t.equal(tones.length, 5, 'sent: sequence length is 5');
  t.deepEqual(tones, ['1', '2', '3', '4', '5'], 'sent: sequence is \'12345\'');

  audioSender.dtmf.stop();

  audioTrack.stop();
  videoTrack.stop();
  pc1.close();
  pc2.close();

  t.end();
});

