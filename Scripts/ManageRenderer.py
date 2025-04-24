#!/usr/bin/env python3
"""
MapleLeaf Engine - Renderer Management Script
This script helps create or remove Renderers in the MapleLeaf engine.
"""

import os
import sys
import re
import shutil
from pathlib import Path

def print_usage():
    """Print usage information for the script."""
    print("Usage:")
    print("  python ManageRenderer.py add RendererName")
    print("  python ManageRenderer.py remove RendererName")
    print("\nExample:")
    print("  python ManageRenderer.py add PostProcessing")

def validate_renderer_name(renderer_name):
    """Validate that the renderer name is appropriate."""
    if not renderer_name:
        print("Error: Renderer name cannot be empty")
        return False
    
    if not re.match(r'^[A-Za-z][A-Za-z0-9]*$', renderer_name):
        print("Error: Renderer name must start with a letter and contain only letters and numbers")
        return False
    
    return True

def add_renderer(renderer_name):
    """Add a new renderer to the engine."""
    project_root = Path(os.path.abspath(os.path.dirname(os.path.dirname(__file__))))
    
    # Check if renderer already exists
    renderer_dir = project_root / "Renderer" / f"{renderer_name}Renderer"
    if renderer_dir.exists():
        print(f"Error: Renderer '{renderer_name}Renderer' already exists at {renderer_dir}")
        return False
    
    # Create renderer directory
    renderer_dir.mkdir(parents=True, exist_ok=True)
    print(f"Created directory: {renderer_dir}")
    
    # Create renderer header file
    header_content = f"""#pragma once

#include "Renderer.hpp"

using namespace MapleLeaf;

namespace MapleLeafApp {{
class {renderer_name}Renderer : public Renderer
{{
public:
    {renderer_name}Renderer();

    void Start() override;
    void Update() override;

private:
    Pipeline::Stage {renderer_name.lower()}Stage;
}};
}}   // namespace MapleLeafApp
"""
    
    with open(renderer_dir / f"{renderer_name}Renderer.hpp", "w") as file:
        file.write(header_content)
    
    # Create renderer source file
    source_content = f"""#include "{renderer_name}Renderer.hpp"
#include "RenderStage.hpp"
#include "config.h"

namespace MapleLeafApp {{
{renderer_name}Renderer::{renderer_name}Renderer()
{{
    // TODO: Configure appropriate attachments and subpasses for this renderer
    std::vector<Attachment> attachments = {{{{0, "swapchain", Attachment::Type::Swapchain, false}}}};
    std::vector<SubpassType> subpasses = {{{{0, {{}}, {{0}}}}}};
    AddRenderStage(std::make_unique<RenderStage>(RenderStage::Type::MONO, attachments, subpasses));
}}

void {renderer_name}Renderer::Start()
{{
    // TODO: Add subrenders to this renderer
    // Example: AddSubrender<{renderer_name}Subrender>({{0, 0}});
}}

void {renderer_name}Renderer::Update() {{}}

}}   // namespace MapleLeafApp
"""
    
    with open(renderer_dir / f"{renderer_name}Renderer.cpp", "w") as file:
        file.write(source_content)
    
    # Update renderer.lua
    renderer_lua_path = project_root / "Renderer" / "Renderer.lua"
    with open(renderer_lua_path, "r") as file:
        lua_content = file.read()
    
    # Check if the renderer is already in the file
    if f"{renderer_name}Renderer" in lua_content:
        print(f"Warning: {renderer_name}Renderer already referenced in Renderer.lua")
    else:
        # Add the new renderer to the file list
        modified_content = lua_content.replace(
            'add_files(',
            f'add_files(\n    "{renderer_name}Renderer/*.cpp",', 
            1
        )
        
        with open(renderer_lua_path, "w") as file:
            file.write(modified_content)
        
        print(f"Updated Renderer.lua to include {renderer_name}Renderer")
    
    # Update xmake.lua to include the new renderer path
    xmake_lua_path = project_root / "xmake.lua"
    with open(xmake_lua_path, "r") as file:
        xmake_content = file.read()
    
    # Look for the renderer include directories section
    renderer_include_pattern = r'add_includedirs\(\s*"Renderer\/DefaultRenderer"'
    if re.search(renderer_include_pattern, xmake_content):
        # Add the new renderer to include paths
        modified_xmake = re.sub(
            renderer_include_pattern,
            f'add_includedirs(\n"Renderer/{renderer_name}Renderer",\n"Renderer/DefaultRenderer"',
            xmake_content
        )
        
        with open(xmake_lua_path, "w") as file:
            file.write(modified_xmake)
        
        print(f"Updated xmake.lua to include {renderer_name}Renderer path")
    else:
        print("Warning: Could not find renderer include section in xmake.lua")
    
    print(f"\nRenderer {renderer_name}Renderer successfully created!")
    print(f"You might want to create appropriate subrenders for this renderer in the RenderPass directory.")
    print("Remember to update MainApp.cpp to use your new renderer.")
    return True

def remove_renderer(renderer_name):
    """Remove an existing renderer from the engine."""
    project_root = Path(os.path.abspath(os.path.dirname(os.path.dirname(__file__))))
    
    # Check if renderer exists
    renderer_dir = project_root / "Renderer" / f"{renderer_name}Renderer"
    if not renderer_dir.exists():
        print(f"Error: Renderer '{renderer_name}Renderer' doesn't exist at {renderer_dir}")
        return False
    
    # Ask for confirmation
    confirm = input(f"Are you sure you want to remove renderer '{renderer_name}Renderer'? (y/n): ")
    if confirm.lower() != 'y':
        print("Operation cancelled.")
        return False
    
    # Remove renderer directory
    shutil.rmtree(renderer_dir)
    print(f"Removed directory: {renderer_dir}")
    
    # Update renderer.lua
    renderer_lua_path = project_root / "Renderer" / "Renderer.lua"
    with open(renderer_lua_path, "r") as file:
        lua_content = file.readlines()
    
    # Remove the renderer from the file list while preserving indentation
    pattern = re.compile(fr'^\s*"{renderer_name}Renderer/\*\.cpp",?\s*\n')
    filtered_content = []
    comma_fixed = False
    
    for i, line in enumerate(lua_content):
        if pattern.match(line):
            # Skip this line as it contains our renderer
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
    
    with open(renderer_lua_path, "w") as file:
        file.writelines(filtered_content)
    
    print(f"Updated Renderer.lua to remove {renderer_name}Renderer")
    
    # Update xmake.lua to remove the renderer path
    xmake_lua_path = project_root / "xmake.lua"
    with open(xmake_lua_path, "r") as file:
        xmake_content = file.readlines()
    
    # Remove the renderer from include paths while preserving indentation
    pattern = re.compile(fr'^\s*"Renderer/{renderer_name}Renderer",?\s*\n')
    filtered_xmake = []
    comma_fixed = False
    
    for i, line in enumerate(xmake_content):
        if pattern.match(line):
            # Skip this line as it contains our renderer path
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
    
    print(f"Updated xmake.lua to remove {renderer_name}Renderer path")
    
    print(f"\nRenderer {renderer_name}Renderer successfully removed!")
    print("Remember to update MainApp.cpp if it was using this renderer.")
    return True

def main():
    """Main entry point for the script."""
    if len(sys.argv) != 3:
        print_usage()
        return 1
    
    action = sys.argv[1].lower()
    renderer_name = sys.argv[2]
    
    if not validate_renderer_name(renderer_name):
        return 1
    
    if action == "add":
        return 0 if add_renderer(renderer_name) else 1
    elif action == "remove":
        return 0 if remove_renderer(renderer_name) else 1
    else:
        print(f"Error: Unknown action '{action}'")
        print_usage()
        return 1

if __name__ == "__main__":
    sys.exit(main())