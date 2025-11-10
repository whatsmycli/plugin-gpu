# GPU Plugin for whatsmycli

Cross-platform GPU detection plugin that displays graphics card information.

## Features

- **Basic Display**: Show active GPU information
- **Multi-GPU Support**: Display all GPUs in the system
- **Index Selection**: Query specific GPU by index
- **Cross-Platform**: Works on Linux, Windows, and macOS

## Usage

```bash
# Show active/default GPU
whatsmy gpu

# Show all GPUs
whatsmy gpu all

# Show specific GPU by index
whatsmy gpu 0
whatsmy gpu 1

# Show help
whatsmy gpu help
```

## Building

### Linux

```bash
mkdir build && cd build
cmake ..
make
```

Output: `linux.so`

### Windows

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

Output: `windows.dll`

### macOS

```bash
mkdir build && cd build
cmake ..
make
```

Output: `macos.dylib`

## Installation

### Manual Installation

Copy the compiled plugin to your whatsmy plugin directory:

**Linux**:
```bash
mkdir -p ~/.local/share/whatsmy/plugins/gpu
cp linux.so ~/.local/share/whatsmy/plugins/gpu/
```

**Windows**:
```powershell
New-Item -ItemType Directory -Path "$env:PROGRAMFILES\whatsmy\plugins\gpu" -Force
Copy-Item windows.dll "$env:PROGRAMFILES\whatsmy\plugins\gpu\"
```

**macOS**:
```bash
mkdir -p /usr/local/lib/whatsmy/plugins/gpu
cp macos.dylib /usr/local/lib/whatsmy/plugins/gpu/
```

### Using Plugin Manager

Once the plugin is published to the plugins repository:

```bash
whatsmy plugin install gpu
```

## Platform-Specific Details

### Linux
- Reads GPU information from `/sys/class/drm/`
- Detects NVIDIA, AMD, and Intel GPUs
- Shows NVIDIA driver version from `/proc/driver/nvidia/version`
- Requires read permissions to `/sys/` filesystem

### Windows
- Uses Windows Setup API for device enumeration
- Queries display adapter information from Device Manager
- Shows manufacturer, device description, and driver info
- Parses PCI hardware IDs for vendor/device codes

### macOS
- Uses IOKit framework (planned)
- Currently shows placeholder information
- Full implementation pending

## Information Displayed

- **Name**: GPU model/description
- **Vendor**: NVIDIA, AMD, Intel, or Unknown
- **Driver Version**: Currently installed driver (when available)
- **PCI ID**: Vendor:Device ID in hexadecimal format
- **Index**: GPU number in multi-GPU systems
- **Status**: Whether the GPU is active/default

## Requirements

- C++17 compatible compiler
- CMake 3.15 or later
- Platform-specific libraries:
  - Linux: Standard C++ library
  - Windows: setupapi.lib
  - macOS: IOKit framework

## Development

This plugin uses the whatsmycli Plugin API v2, which supports argument passing.

API signature:
```cpp
extern "C" int plugin_run(int argc, char* argv[]);
```

## License

GPLv3 - See LICENSE file

## Contributing

Contributions welcome! Areas for improvement:
- Enhanced GPU name resolution using PCI ID databases
- Memory size detection
- Temperature monitoring
- Clock speed information
- Power consumption data
- macOS IOKit implementation

