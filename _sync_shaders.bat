@echo off

for /r %%v in (data\shaders\*.vert,data\shaders\*.frag) do (
    if exist %%v.spv del %%v.spv
	%home%\dev\lib\VulkanSDK\1.2.162.1\Bin32\glslc.exe %%v -o %%v.spv
	echo compiled %%~nxv to %%~nxv.spv
)
