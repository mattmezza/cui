# Optional macOS Speech recognizer proposal

Design checkpoint only; no adapter source or working native recognizer is
shipped here yet. See [the design, coverage and implementation slices](../../docs/design/SPEECH_MACOS_RECOGNIZER.md).

The proposed adapter uses existing `cui_speech_recognizer`, a bounded retained
callback mailbox and owner-thread polling. Speech permission preparation is an
explicit user action before starting a session. Microphone consent remains a
separate application/SDL input concern. Both on-device support and an on-device
request are mandatory; no network fallback is planned.

Production implementation requires integrator review of this checkpoint.
Automatic validation will use injected services and synthetic PCM, never real
permission requests, microphone access, windows or live recognition. macOS
hardware/consent/recognition remain deferred.
