set_project("MapleLeaf")
set_version("1.0.0", {build = "%Y%m%d%H%M"})
set_languages("c++17")
set_arch("x64")
add_rules("mode.debug", "mode.release", "mode.releasedbg")
add_rules("plugin.compile_commands.autoupdate", {outputdir = ".vscode/"})
if is_plat("windows") then 
    set_toolchains("msvc")
elseif is_plat("linux") then
    set_toolchains("clang")
end

rootPath = os.projectdir():gsub("\\", "\\\\")
add_includedirs("Config/")
set_configvar("VERSION_MAJOR", 1)
set_configvar("VERSION_MINOR", 0)
set_configvar("VERSION_ALTER", 0)
set_configvar("PROJECT_DIR", rootPath)
set_configvar("MAPLELEAF_SCENE_DEBUG", false)
set_configvar("MAPLELEAF_SHADER_DEBUG", true)
set_configvar("MAPLELEAF_DEVICE_DEBUG", false)
set_configvar("MAPLELEAF_GRAPHIC_DEBUG", false)
set_configvar("MAPLELEAF_GPUSCENE_DEBUG", false)
set_configvar("MAPLELEAF_PIPELINE_DEBUG", false)
set_configvar("MAPLELEAF_VALIDATION_DEBUG", true)
set_configvar("MAPLELEAF_DESCRIPTOR_DEBUG", false)
set_configvar("MAPLELEAF_RENDERSTAGE_DEBUG", false)
set_configvar("MAPLELEAF_RAY_TRACING", false)
set_configvar("SHADOW_MAP_SIZE", 1024)
set_configdir("Config") 
add_configfiles("./config.h.in")

add_requires("glm", "glfw", "assimp", "stb", "boost", "nlohmann_json")
add_requires("volk 1.4.304", "spirv-reflect 1.4.304", "glslang 1.4.304", {verify = false})
add_requires("imgui", {configs = {glfw_vulkan = true}})
add_requires("freeimage", {configs = {shared = true}})
add_packages("glm", "glfw", "volk", "spirv-tools", "spirv-reflect", "glslang", "assimp", "stb", "boost", "imgui", "freeimage", "nlohmann_json")

add_includedirs(
"Core/Devices", 
"Core/Utils", 
"Core/Maths", 
"Core/Engine",
"Core/Graphics", 
"Core/Graphics/Instance", 
"Core/Graphics/Devices", 
"Core/Graphics/Swapchain", 
"Core/Graphics/Resources", 
"Core/Graphics/Renderer", 
"Core/Graphics/Commands", 
"Core/Graphics/Descriptors/",
"Core/Graphics/Pipelines", 
"Core/Graphics/AccelerationStruct",
"Core/Files", 
"Core/Models", 
"Core/Materials", 
"Core/Resources", 
"Core/Meshes", 
"Core/Scenes", 
"Core/Scenes/Light", 
"Core/Scenes/Shadow", 
"Core/Scenes/Skybox", 
"Core/Scenes/Animation",
"Core/Importers", 
"Core/Importers/Builder",
"Core/Bitmaps",
"Core/Inputs",
"Core/Imgui",
"Core/Utils/SampleGenerators",
"Core/DerivedScene",
"Core/DerivedScene/ASScene",
"Core/DerivedScene/GPUScene"
)

add_includedirs(
"RenderPass/Shadow",
"RenderPass",
"RenderPass/GBuffer",
"RenderPass/Deferred",
"RenderPass/Imgui",
"RenderPass/Default",
"RenderPass/Skybox",
"RenderPass/SkyboxMapping",
"RenderPass/ToneMapping",
"RenderPass/Resolved"
)
add_includedirs(
"Renderer/DeferredRenderer",
"Renderer/DefaultRenderer",
"Renderer/SkyboxMappingRenderer"
)

-- Include component xmake files
includes("RenderPass/RenderPass.lua")
includes("Core/Core.lua")
includes("Renderer/Renderer.lua")
target("MapleLeaf")
    set_kind("binary")
    add_deps("Core")
    add_deps("RenderPass")
    add_deps("Renderer")
    add_includedirs("App/")
    add_files("App/*.cpp")
    if is_plat("windows") then
        add_syslinks("Comdlg32") -- Adding Windows Common Dialogs Library.
    end
target_end()
