#!/bin/bash
# Generates the Xcode project for HALO on iPad (AUv3 for Logic Pro for iPad, GarageBand, AUM…).
#
# Apple does not allow installer files for iPad apps/plugins. To get HALO on your iPad:
#   1. Run this script on a Mac with Xcode:   ./packaging/ios/build_ipad.sh
#   2. Open build-ios/HBHalo.xcodeproj in Xcode.
#   3. Select the "HBHalo_Standalone" scheme, then Signing & Capabilities > Team (your Apple ID)
#      for BOTH the Standalone and the AUv3 targets.
#   4. Plug in the iPad, pick it as the run destination, press Run.
#      - Free Apple ID: the app works for 7 days, then re-run from Xcode.
#      - Paid Apple Developer account ($99/yr): use Product > Archive > TestFlight / App Store.
#   5. On the iPad open the HALO app once, then in Logic Pro for iPad add it from Plug-ins > Audio Units > HALO Studios.
set -euo pipefail
cd "$(dirname "$0")/../.."
if [ ! -d JUCE ]; then git clone --depth 1 --branch 9.0.5 https://github.com/juce-framework/JUCE.git JUCE; fi
cmake -S . -B build-ios -G Xcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
  -DCMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY=2
# compile check without signing (signing happens in Xcode with your Apple ID)
cmake --build build-ios --config Release --target HBHalo_AUv3 -- -sdk iphoneos CODE_SIGNING_ALLOWED=NO -quiet
echo "Xcode project ready: build-ios/HBHalo.xcodeproj"
