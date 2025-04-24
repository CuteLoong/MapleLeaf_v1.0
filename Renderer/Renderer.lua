target("Renderer")
    set_kind("static")
    add_files(
    "DeferredRenderer/*.cpp",
    "DefaultRenderer/*.cpp", 
    "SkyboxMappingRenderer/*.cpp"
    )
target_end()