forfiles /s /m *.glsl /c "cmd /c %VULKAN_SDK%/Bin/glslangValidator.exe -V @fname.glsl -o ../../../../../shaders/@fname.spv"
pause