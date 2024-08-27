forfiles /s /m *.vert /c "cmd /c %VULKAN_SDK%/Bin/glslangValidator.exe @path -gVS -V -o @fname.vert.spv"
forfiles /s /m *.frag /c "cmd /c %VULKAN_SDK%/Bin/glslangValidator.exe @path -gPS -V -o @fname.frag.spv"
forfiles /s /m *.comp /c "cmd /c %VULKAN_SDK%/Bin/glslangValidator.exe @path -gCS -V -o @fname.comp.spv"
pause