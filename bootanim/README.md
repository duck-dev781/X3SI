# Chase's Dev Box boot animation

This is a separate LibXenon homebrew target inside X3SI.

Sequence:

1. Xbox 360 console icon
2. BadAvatar profile icon
3. Byrom90 XeUnshackle chain
4. Phoenix logo
5. Repeat the four-icon sequence five times, faster each cycle
6. Glitch/explosion transition
7. Final Xbox + wrench + USB + gear screen
8. `Chase's Dev Box`

The existing X3SI target is unchanged.

The two supplied artwork images are embedded into the ELF32 at build time. The current graphics path targets 1280x720 HDMI, matching the Xbox 360 development setup used for this project.

LibXenon officially builds bare-metal Xbox 360 homebrew and its toolchain supports the same Docker build approach already used by X3SI. citeturn0search0

This target currently produces `ChasesDevBoxBoot.elf32`. XEX packaging is kept separate because LibXenon itself produces ELF32 homebrew; a real ELF-to-XEX conversion step must be added and verified independently.
