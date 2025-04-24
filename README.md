# MapleLeaf v1.0

This is a simplified version of the MapleLeaf rendering engine, focused on basic triangle rendering functionality.

## Prerequisties
+ Visual Studio Code
+ Xmake 2.8.0 or newer version
+ Vulkan SDK 1.3 or higher version

## Build MapleLeaf
``` shell
git clone https://github.com/CuteLoong/MapleLeaf_v1.0.git
xmake f -m release[/debug]
xmake

xmake run
```
For more details about xmake, see: [xmake](https://xmake.io/#/zh-cn/getting_started)

## Usage

Add **Renderer**:
``` shell
python Scripts\ManageRenderer.py add [RendererName]
```

Remove **Renderer**:
``` shell
python Scripts\ManageRenderer.py remove [RendererName]
```

Add **RenderPass**:
``` shell
python Scripts\ManageRenderPass.py add [RenderPassName]
```

Remove **RenderPass**:
``` shell
python Scripts\ManageRenderPass.py remove [RenderPassName]
```

## Project Structure

- `App`: Main application entry point
- `RenderPass`: rendering pass implementation
- `Renderer`: Renderer implementation for rendering.
- `Resources`: Render Resources, such as shaders, skybox.

## Renference

+ Acid (https://github.com/EQMG/Acid/tree/1bd67dc8c2c2ebee2ca95f0cabb40b926f747fcf)

+ Falcor (https://github.com/NVIDIAGameWorks/Falcor)