param(
    [string]$VulkanSdk = $env:VULKAN_SDK
)

$ErrorActionPreference = 'Stop'
$compiler = Join-Path $VulkanSdk 'Bin\glslc.exe'
if (-not (Test-Path -LiteralPath $compiler)) {
    throw "glslc.exe was not found. Set VULKAN_SDK to a Vulkan SDK installation."
}

$shaderDirectory = Join-Path $PSScriptRoot '..\assets\shaders'
& $compiler (Join-Path $shaderDirectory 'standard_vk.vert') '-o' (Join-Path $shaderDirectory 'standard_vk.vert.spv')
if ($LASTEXITCODE -ne 0) { throw 'Vertex shader compilation failed.' }
& $compiler (Join-Path $shaderDirectory 'standard_vk.frag') '-o' (Join-Path $shaderDirectory 'standard_vk.frag.spv')
if ($LASTEXITCODE -ne 0) { throw 'Fragment shader compilation failed.' }
& $compiler (Join-Path $shaderDirectory 'grid_vk.vert') '-o' (Join-Path $shaderDirectory 'grid_vk.vert.spv')
if ($LASTEXITCODE -ne 0) { throw 'Grid vertex shader compilation failed.' }
& $compiler (Join-Path $shaderDirectory 'grid_vk.frag') '-o' (Join-Path $shaderDirectory 'grid_vk.frag.spv')
if ($LASTEXITCODE -ne 0) { throw 'Grid fragment shader compilation failed.' }
