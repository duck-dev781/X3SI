# Chase's Dev Box Boot Splash

This folder contains the separate Xbox 360 LibXenon boot-splash animation target.

The main X3SI firmware is built by the normal X3SI workflow and its artifact is kept separate.

Sequence:

1. Xbox 360 console icon
2. BadAvatar profile icon
3. Byrom90 XeUnshackle chain
4. Phoenix logo
5. Repeat the four-icon sequence five times, faster each cycle
6. Glitch/explosion transition
7. Final Xbox + wrench + USB + gear screen
8. Chase's Dev Box

The artwork is embedded into the ELF32 at build time.

The current build produces ChasesDevBoxBoot.elf32. This is the working LibXenon build format; XEX packaging is kept separate until a verified ELF32-to-XEX conversion step is available.
