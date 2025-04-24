#!/usr/bin/env python3
"""
MapleLeaf Engine - RenderPass Management Script
This script helps create or remove RenderPasses in the MapleLeaf engine.
"""

import os
import sys
import re
import shutil
from pathlib import Path

def print_usage():
    """Print usage information for the script."""
    print("Usage:")
    print("  python ManageRenderPass.py add RenderPassName")
    print("  python ManageRenderPass.py remove RenderPassName")
    print("\nExample:")
    print("  python ManageRenderPass.py add Bloom")

def validate_renderpass_name(renderpass_name):
    """Validate that the renderpass name is appropriate."""
    if not renderpass_name:
        print("Error: RenderPass name cannot be empty")
        return False
    
    if not re.match(r'^[A-Za-z][A-Za-z0-9]*$', renderpass_name):
        print("Error: RenderPass name must start with a letter and contain only letters and numbers")
        return False
    
    return True

def add_renderpass(renderpass_name):
    """Add a new renderpass to the engine."""
    project_root = Path(os.path.abspath(os.path.dirname(os.path.dirname(__file__))))
    
    # Check if renderpass already exists
    renderpass_dir = project_root / "RenderPass" / renderpass_name
    if renderpass_dir.exists():
        print(f"Error: RenderPass '{renderpass_name}' already exists at {renderpass_dir}")
        return False
    
    # Create renderpass directory
    renderpass_dir.mkdir(parents=True, exist_ok=True)
    print(f"Created directory: {renderpass_dir}")
    
    # Create subrender header file
    header_content = f"""#pragma once

#include "DescriptorHandler.hpp"
#include "PipelineGraphics.hpp"
#include "Subrender.hpp"
#include "UniformHandler.hpp"

namespace MapleLeaf {{
class {renderpass_name}Subrender : public Subrender
{{
public:
    explicit {renderpass_name}Subrender(const Pipeline::Stage& stage);
    ~{renderpass_name}Subrender() override = default;
    
    void PreRender(const CommandBuffer& commandBuffer) override;
    void Render(const CommandBuffer& commandBuffer) override;
    void PostRender(const CommandBuffer& commandBuffer) override;
    
    void RegisterImGui() override;

private:
    PipelineGraphics pipeline;
    DescriptorsHandler descriptorSet;
    UniformHandler uniformObject;
}};
}}   // namespace MapleLeaf
"""
    
    with open(renderpass_dir / f"{renderpass_name}Subrender.hpp", "w") as file:
        file.write(header_content)
    
    # Create subrender source file
    source_content = f"""#include "{renderpass_name}Subrender.hpp"
#include "Graphics.hpp"
#include "Scenes.hpp"

namespace MapleLeaf {{
{renderpass_name}Subrender::{renderpass_name}Subrender(const Pipeline::Stage& stage)
    : Subrender(stage)
{{
    // TODO: Configure pipeline shaders, descriptors, and uniforms
    pipeline.SetShaders({{% std::filesystem::path{{
        "Shader/{renderpass_name}/{renderpass_name}.vert"}},
        std::filesystem::path{{
        "Shader/{renderpass_name}/{renderpass_name}.frag"}} %}});

    pipeline.Create({{0}});
}}

void {renderpass_name}Subrender::PreRender(const CommandBuffer& commandBuffer)
{{
    // TODO: Implement pre-render logic if needed
}}

void {renderpass_name}Subrender::Render(const CommandBuffer& commandBuffer)
{{
    // Skip if pipeline not created or disabled
    if (!pipeline || !IsEnabled())
        return;
    
    // TODO: Implement render logic
    const auto& renderStage = Graphics::Get()->GetRenderStage(GetStage().first);
    pipeline.BindPipeline(commandBuffer);

    // Set any descriptors and push constants here
    // descriptorSet.BindDescriptor(commandBuffer, pipeline);
    
    // Draw with your specific commands
    // vkCmdDraw(commandBuffer, vertexCount, instanceCount, 0, 0);
}}

void {renderpass_name}Subrender::PostRender(const CommandBuffer& commandBuffer)
{{
    // TODO: Implement post-render logic if needed
}}

void {renderpass_name}Subrender::RegisterImGui()
{{
    // TODO: Implement ImGui controls if needed
}}

}}   // namespace MapleLeaf
"""
    
    with open(renderpass_dir / f"{renderpass_name}Subrender.cpp", "w") as file:
        file.write(source_content)
    
    # Create shader directory
    shader_dir = project_root / "Resources" / "Shader" / renderpass_name
    shader_dir.mkdir(parents=True, exist_ok=True)
    print(f"Created shader directory: {shader_dir}")
    
    # Create basic vertex shader
    vert_shader = f"""#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec2 outUV;

void main() 
{{
    outUV = inUV;
    gl_Position = vec4(inPosition, 1.0);
}}
"""
    with open(shader_dir / f"{renderpass_name}.vert", "w") as file:
        file.write(vert_shader)
    
    # Create basic fragment shader
    frag_shader = f"""#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0, binding = 0) uniform UniformObject {{
    vec4 color;
}} ubo;

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

void main() 
{{
    // TODO: Implement shader logic
    outColor = ubo.color;
}}
"""
    with open(shader_dir / f"{renderpass_name}.frag", "w") as file:
        file.write(frag_shader)
    
    # Update RenderPass.lua
    renderpass_lua_path = project_root / "RenderPass" / "RenderPass.lua"
    with open(renderpass_lua_path, "r") as file:
        lua_content = file.read()
    
    # Check if the renderpass is already in the file
    if f"{renderpass_name}/" in lua_content:
        print(f"Warning: {renderpass_name} already referenced in RenderPass.lua")
    else:
        # Add the new renderpass to the file list
        modified_content = lua_content.replace(
            'add_files(',
            f'add_files(\n    "{renderpass_name}/*.cpp",', 
            1
        )
        
        with open(renderpass_lua_path, "w") as file:
            file.write(modified_content)
        
        print(f"Updated RenderPass.lua to include {renderpass_name}")
    
    # Update xmake.lua to include the new renderpass path
    xmake_lua_path = project_root / "xmake.lua"
    with open(xmake_lua_path, "r") as file:
        xmake_content = file.read()
    
    # Look for the renderpass include directories section
    renderpass_include_pattern = r'add_includedirs\(\s*\n"RenderPass"'
    if re.search(renderpass_include_pattern, xmake_content):
        # Add the new renderpass to include paths
        modified_xmake = re.sub(
            renderpass_include_pattern,
            f'add_includedirs(\n"RenderPass/{renderpass_name}",\n"RenderPass"',
            xmake_content
        )
        
        with open(xmake_lua_path, "w") as file:
            file.write(modified_xmake)
        
        print(f"Updated xmake.lua to include {renderpass_name} path")
    else:
        print("Warning: Could not find renderpass include section in xmake.lua")
    
    print(f"\nRenderPass {renderpass_name} successfully created!")
    print(f"Basic shader files have been created in {shader_dir}")
    print("Remember to create a Renderer that uses this RenderPass or add it to an existing Renderer")
    return True

def remove_renderpass(renderpass_name):
    """Remove an existing renderpass from the engine."""
    project_root = Path(os.path.abspath(os.path.dirname(os.path.dirname(__file__))))
    
    # Check if renderpass exists
    renderpass_dir = project_root / "RenderPass" / renderpass_name
    if not renderpass_dir.exists():
        print(f"Error: RenderPass '{renderpass_name}' doesn't exist at {renderpass_dir}")
        return False
    
    # Ask for confirmation
    confirm = input(f"Are you sure you want to remove renderpass '{renderpass_name}'? (y/n): ")
    if confirm.lower() != 'y':
        print("Operation cancelled.")
        return False
    
    # Check for associated shader directory
    shader_dir = project_root / "Resources" / "Shader" / renderpass_name
    shader_exists = shader_dir.exists()
    
    # Ask about shader removal if exists
    remove_shaders = False
    if shader_exists:
        confirm_shader = input(f"Do you also want to remove shader directory '{renderpass_name}'? (y/n): ")
        remove_shaders = confirm_shader.lower() == 'y'
    
    # Remove renderpass directory
    shutil.rmtree(renderpass_dir)
    print(f"Removed directory: {renderpass_dir}")
    
    # Remove shader directory if confirmed
    if shader_exists and remove_shaders:
        shutil.rmtree(shader_dir)
        print(f"Removed shader directory: {shader_dir}")
    
    # Update RenderPass.lua
    renderpass_lua_path = project_root / "RenderPass" / "RenderPass.lua"
    with open(renderpass_lua_path, "r") as file:
        lua_content = file.readlines()
    
    # Remove the renderpass from the file list while preserving indentation
    pattern = re.compile(fr'^\s*"{renderpass_name}/\*\.cpp",?\s*\n')
    filtered_content = []
    comma_fixed = False
    
    for i, line in enumerate(lua_content):
        if pattern.match(line):
            # Skip this line as it contains our renderpass
            # If the previous line ended with a comma and the next line doesn't have a comma
            # then we need to keep the comma on the previous line
            if i > 0 and i < len(lua_content) - 1:
                prev_line = lua_content[i-1]
                next_line = lua_content[i+1]
                if prev_line.strip().endswith(',') and not next_line.strip().endswith(',') and not next_line.strip().endswith(')'):
                    # Remove the trailing comma from the previous line
                    if not comma_fixed:
                        filtered_content[-1] = prev_line.rstrip().rstrip(',') + '\n'
                        comma_fixed = True
        else:
            filtered_content.append(line)
    
    with open(renderpass_lua_path, "w") as file:
        file.writelines(filtered_content)
    
    print(f"Updated RenderPass.lua to remove {renderpass_name}")
    
    # Update xmake.lua to remove the renderpass path
    xmake_lua_path = project_root / "xmake.lua"
    with open(xmake_lua_path, "r") as file:
        xmake_content = file.readlines()
    
    # Remove the renderpass from include paths while preserving indentation
    pattern = re.compile(fr'^\s*"RenderPass/{renderpass_name}",?\s*\n')
    filtered_xmake = []
    comma_fixed = False
    
    for i, line in enumerate(xmake_content):
        if pattern.match(line):
            # Skip this line as it contains our renderpass path
            # Handle comma fixing like above
            if i > 0 and i < len(xmake_content) - 1:
                prev_line = xmake_content[i-1]
                next_line = xmake_content[i+1]
                if prev_line.strip().endswith(',') and not next_line.strip().endswith(',') and not next_line.strip().endswith(')'):
                    # Remove the trailing comma from the previous line
                    if not comma_fixed:
                        filtered_xmake[-1] = prev_line.rstrip().rstrip(',') + '\n'
                        comma_fixed = True
        else:
            filtered_xmake.append(line)
    
    with open(xmake_lua_path, "w") as file:
        file.writelines(filtered_xmake)
    
    print(f"Updated xmake.lua to remove {renderpass_name} path")
    
    print(f"\nRenderPass {renderpass_name} successfully removed!")
    print("Remember to update any Renderers that were using this RenderPass.")
    return True

def main():
    """Main entry point for the script."""
    if len(sys.argv) != 3:
        print_usage()
        return 1
    
    action = sys.argv[1].lower()
    renderpass_name = sys.argv[2]
    
    if not validate_renderpass_name(renderpass_name):
        return 1
    
    if action == "add":
        return 0 if add_renderpass(renderpass_name) else 1
    elif action == "remove":
        return 0 if remove_renderpass(renderpass_name) else 1
    else:
        print(f"Error: Unknown action '{action}'")
        print_usage()
        return 1

if __name__ == "__main__":
    sys.exit(main())