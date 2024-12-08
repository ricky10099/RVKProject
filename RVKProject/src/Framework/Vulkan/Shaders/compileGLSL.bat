forfiles /s /m *.glsl /c "cmd /c %VULKAN_SDK%/Bin/glslangValidator.exe @path -gVS -V -o ../../../../../shaders/@fname.spv"
pause