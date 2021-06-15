@echo off

del data\shaders\*.spv

for /r %%v in (data\shaders\*.vert,data\shaders\*.frag) do (
	%VULKAN_SDK%\Bin32\glslc.exe %%v -o %%v.spv
	echo compiled %%~nxv to %%~nxv.spv
)
