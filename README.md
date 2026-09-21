# steamdeck-pipewire-capture
A lightweight utility example to capture screenshots directly from Gamescope video streams vie PipeWire on the SteamDeck

# Why the normal screenshot tools fail here

Taking a screenshot on Linux used to be simple, but standard tools like scrot or grim just don't work in the Steam Deck's Gaming Mode.

* Wayland blocks screen snooping: On old Linux systems (X11), any app could grab what was on your screen anytime. Wayland locks this down for security, so windows aren't allowed to see each other by default.

* Gamescope isn't a regular desktop: Gaming Mode runs Gamescope, a stripped-down compositor built purely for gaming performance. It doesn't include the desktop APIs that standard screenshot utilities expect to find.


Instead, Gamescope shares screen frames through PipeWire—the system's multimedia layer. This utility simply taps straight into that video feed, snatches a single frame from memory, and saves it as a PNG.
