forfiles /P src\Framework\Vulkan\Shaders /s /m *.glsl /c "cmd /c %VULKAN_SDK%\Bin\glslangValidator.exe -V @fname.glsl -o ../../../../../shaders/@fname.spv"
pause