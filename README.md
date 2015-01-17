# ndk_handmade

This is an implementation of an Android platform layer for [Handmade Hero](http://handmadehero.org/),
using the [Android Native Development Kit (NDK)](https://developer.android.com/tools/sdk/ndk/index.html).

# Prerequisites

You'll obviously need the Handmade Hero source code for the platform-indepenent game code and the
game assets.

Install Android Studio and the Android NDK.

# Build process

Copy the Handmade Hero source files into `ndk_handmade/mobile/src/main/handmade`.

Copy the Handmade Hero test assets into `ndk_handmade/mobile/src/assets/test`.

Open in Android Studio and configure local.properties file with links to the Android SDK and Android NDK:

    sdk.dir=S\:\\
    ndk.dir=U\:\\

Press the "Play" button, and select a device or emulator.

# Implementation progress

Completed (at least partially):

* Game memory allocation
* Graphics
* Frame timing and locking
* Debug platform function - enough to read the test assets
* Calling UpdateAndRender

Still needed:

* Input - touch, key, controller, ...
* Audio

Not planned:

* Hot reloading
* Save state/record/replay