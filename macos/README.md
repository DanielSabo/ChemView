### Steps to build the MacOS app bundle:

**1. Build the prefixes:**
> ./generate_prefix.py --x86 ~/homebrew-x86/ --arm ~/homebrew-arm/

**2. Build the application in QtCreator.**

**3. Bundle the Qt libraries:**
> /Applications/Qt/6.2.2/macos/bin/macdeployqt ../../build-ChemView-Qt_6_2_2_for_macOS-Debug/ChemView.app -no-strip -dmg -codesign=-