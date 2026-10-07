# X3SI — Xbox 360 System Information

X3SI is an Xbox 360 homebrew system-information utility built around Free60 LibXenon.

It reads hardware/runtime values from the console instead of hard-coding a known console configuration.

Detected values include motherboard/platform, CPU PVR, Xenos GPU ID, PCI bridge revision, RAM, CPU/GPU/eDRAM/chassis temperatures, SMC fan target/version, RTC, DVD tray, A/V pack, NAND metadata, and mounted storage.

Storage reports contain total capacity in bytes, used bytes and percentage, free bytes and percentage, plus grand totals. Unavailable values are shown as N/A.

Controls:
- A = Celsius/Fahrenheit
- X = storage selector, then save report
- Y = refresh
- B = exit
- Xbox Guide = exit

The report filename is SystemInfo_YYYY-MM-DD_HH-MM-SS.txt when the Xbox RTC is set.

Build with the Free60 LibXenon toolchain.

Important: LibXenon natively produces ELF32. A dashboard-launchable XEX requires a separate ELF-to-XEX packager; this repository does not fake that by renaming an ELF.

The X button uses a real device selector over LibXenon's mounted storage devices. The retail dashboard XAM storage-selector UI is not exposed to a bare-metal LibXenon application, so X3SI does not pretend to invoke that unavailable UI.
