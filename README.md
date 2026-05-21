# Vulkan renderer

This renderer is my attempt at Khornos' updated Vulkan tutorial. 
It possibly diverges from the tutorial in many places. 
The tutorial and other Vulkan tutorials like vkGuide largely tell you what to code instead of telling how to code.
I have tried to remedy that by extensively referring to the spec and adding annotations as comments in the critical parts.

# Build instructions

GLFW, Vulkan SDK, and pkg-config are required dependencies. 
1. Use `make isMacOS = false` for devices that natively support Vulkan, otherwise `make` on MacOS, for a debug build. 
2. Use `make [isMacOS = ...] isDebug = false` for optimised build without validation layers.
3. Run using `cd build && ./renderer`, or `chmod + x ./run.sh` `./run.sh` in *-nix.
