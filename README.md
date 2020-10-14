# GlobalMidi

This provides a "GlobalMidi" Max object for Max/MSP to recieve raw midi data from a device. The device can be specified with the `@device` attribute for example `GlobalMidi @device "Midi Fighter Twister"`.
The idea is to work around a limitation in Max for Live where notein, ctlin, etc. aren't available and MIDI input should be received with no regard to track arm status.

## Platform availablility

GlobalMidi is **only** available for macOS.
