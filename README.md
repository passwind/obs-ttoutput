# OBS TTOutput Plugin

A professional OBS Studio plugin that provides advanced output management capabilities with a modern Qt6-based user interface. This plugin enables streamlined RTMP streaming and file recording with comprehensive source selection and real-time monitoring features.

## Features

### 🎥 Multiple Output Types
- **RTMP Streaming**: Direct streaming to RTMP servers with customizable URL and stream key
- **File Recording**: Local file recording with support for multiple formats (MP4, MKV, FLV)

### 🎛️ Advanced Configuration
- **Video Encoder Settings**: Support for H.264 (x264) and H.265 (x265) codecs
- **Quality Control**: Configurable bitrate, resolution (width/height), and frame rate
- **Encoder Presets**: Multiple quality presets for optimal performance
- **Source Selection**: Granular control over audio and video sources

### 🖥️ Modern User Interface
- **Tabbed Interface**: Organized configuration, source selection, and status monitoring
- **Real-time Status**: Live monitoring of output status, bitrate, and performance metrics
- **Progress Tracking**: Visual progress indicators for encoding operations
- **Configuration Management**: Save, load, and manage multiple output configurations

### 📊 Monitoring & Analytics
- **Performance Metrics**: Real-time CPU usage and dropped frame monitoring
- **Bitrate Monitoring**: Live bitrate tracking and statistics
- **Status Indicators**: Clear visual feedback for output state and health

## Requirements

### System Requirements
- **OBS Studio**: Version 30.0 or later
- **Operating System**: 
  - macOS 10.15 (Catalina) or later
  - Windows 10 (version 1903) or later
  - Linux (Ubuntu 20.04 LTS or equivalent)

### Development Dependencies
- **CMake**: Version 3.28 or later
- **C++17 compatible compiler**
- **Git**: For cloning the repository

**Note**: Qt6, OBS Studio development libraries, and other dependencies are automatically managed through the OBS plugin template's dependency system. You do **NOT** need to install Qt6 or OBS development libraries manually.

## Building from Source

### Prerequisites

The OBS plugin template automatically handles all dependencies including Qt6, OBS Studio libraries, and other required components through its `.deps` directory system. You only need basic build tools:

1. **Install Basic Build Tools**
   ```bash
   # macOS
   xcode-select --install
   
   # Ubuntu/Debian
   sudo apt update
   sudo apt install build-essential git cmake
   
   # Windows
   # Install Visual Studio 2019/2022 with C++ development tools
   # Install Git for Windows
   # Install CMake
   ```

2. **Dependencies Auto-Management**
   - All dependencies (Qt6, OBS libraries, etc.) are automatically downloaded and configured
   - The `.deps` directory will be created during the first build
   - No manual installation of Qt6 or OBS development libraries required

### Build Instructions

1. **Clone the Repository**
   ```bash
   git clone git@github.com:passwind/obs-ttoutput.git
   cd obs-ttoutput
   ```

2. **Configure the Build** (Dependencies Auto-Download)
   ```bash
   # macOS
   cmake --preset macos
   
   # Linux
   cmake --preset linux
   
   # Windows
   cmake --preset windows
   ```
   
   **Note**: During the first configuration, CMake will automatically:
   - Download and configure Qt6
   - Download OBS Studio development libraries
   - Set up all required dependencies in the `.deps` directory
   - This process may take several minutes on the first run

3. **Build the Plugin**
   ```bash
   # macOS
   cmake --build --preset macos --config Debug
   
   # Linux
   cmake --build --preset linux --config Debug
   
   # Windows
   cmake --build --preset windows --config Debug
   ```

4. **Locate the Built Plugin**
   ```bash
   # The built plugin will be located in:
   # macOS: build_macos/Debug/obs-ttoutput.plugin
   # Linux: build_linux/Debug/obs-ttoutput.so
   # Windows: build_windows/Debug/obs-ttoutput.dll
   ```

5. **Install the Plugin** (Optional)
   ```bash
   # Copy to your OBS plugins directory:
   # macOS: ~/Library/Application Support/obs-studio/plugins/
   # Windows: %APPDATA%/obs-studio/plugins/
   # Linux: ~/.config/obs-studio/plugins/
   ```

### Dependency Management

This plugin uses the OBS plugin template's automatic dependency management system:

- **`.deps` Directory**: All dependencies are downloaded and stored in this directory
- **Automatic Updates**: Dependencies are automatically updated when needed
- **Cross-Platform**: Works consistently across macOS, Windows, and Linux
- **No Manual Setup**: No need to manually install Qt6, OBS libraries, or other dependencies
- **Clean Builds**: Delete the `.deps` directory to force a fresh download of all dependencies

**Important Notes**:
- The `.deps` directory can be large (several GB) as it contains all required libraries
- First build will take longer due to dependency downloads
- Subsequent builds will be much faster as dependencies are cached
- The `.deps` directory is automatically excluded from git via `.gitignore`

## Installation

### Automatic Installation
1. Download the latest release from the [Releases](https://github.com/passwind/obs-ttoutput/releases) page
2. Extract the plugin files to your OBS plugins directory:
   - **macOS**: `~/Library/Application Support/obs-studio/plugins/`
   - **Windows**: `%APPDATA%/obs-studio/plugins/`
   - **Linux**: `~/.config/obs-studio/plugins/`
3. Restart OBS Studio

### Manual Installation
1. Build the plugin from source (see Building from Source)
2. Copy the built plugin to your OBS plugins directory
3. Restart OBS Studio

## Usage

### Getting Started

1. **Open the Plugin Interface**
   - Launch OBS Studio
   - Navigate to `View` → `Docks` → `TTOutput`
   - The TTOutput dock will appear in your OBS interface

2. **Configure Output Settings**
   - Click the **Configuration** tab
   - Select your output type (RTMP or File)
   - Configure the appropriate settings for your use case

3. **Select Sources**
   - Switch to the **Sources** tab
   - Choose which video and audio sources to include in your output
   - Adjust volume levels as needed

4. **Monitor Status**
   - Use the **Status** tab to monitor your output
   - View real-time metrics including bitrate, CPU usage, and dropped frames
   - Start/stop output using the control buttons

### Configuration Options

#### RTMP Streaming
- **Server URL**: Your RTMP server endpoint
- **Stream Key**: Your unique stream key
- **Video Codec**: H.264 or H.265
- **Bitrate**: Target bitrate in kbps
- **Resolution**: Output width and height
- **Frame Rate**: Target FPS

#### File Recording
- **Output Path**: Destination file location
- **File Format**: MP4, MKV, or FLV
- **Video Codec**: H.264 or H.265
- **Quality Settings**: Bitrate, resolution, and frame rate
- **Encoder Preset**: Quality vs. performance balance

### Advanced Features

#### Source Management
- **Video Sources**: Select from available video sources in your OBS scene
- **Audio Sources**: Choose audio sources with individual volume control
- **Real-time Preview**: Monitor selected sources before starting output

#### Performance Monitoring
- **CPU Usage**: Real-time CPU utilization tracking
- **Dropped Frames**: Monitor encoding performance
- **Bitrate Statistics**: Current and average bitrate information
- **Output Status**: Clear indicators for output state

## User Interface Components

### Configuration Tab
- **Output Type Selection**: Choose between RTMP and File output
- **Encoder Settings**: Configure video codec, bitrate, resolution, and frame rate
- **Quality Presets**: Quick selection of optimized encoding settings
- **Advanced Options**: Fine-tune encoder parameters

### Sources Tab
- **Video Source List**: Available video sources with selection checkboxes
- **Audio Source List**: Available audio sources with volume sliders
- **Source Preview**: Real-time preview of selected sources
- **Refresh Button**: Update source lists

### Status Tab
- **Output Controls**: Start/Stop buttons with status indicators
- **Performance Metrics**: Real-time CPU usage and dropped frame counters
- **Bitrate Monitor**: Current and average bitrate display
- **Progress Indicators**: Visual feedback for encoding operations

## Troubleshooting

### Common Issues

1. **Plugin Not Loading**
   - Verify OBS Studio version compatibility
   - Check plugin installation directory
   - Review OBS log files for error messages

2. **Encoding Errors**
   - Verify encoder settings are within system capabilities
   - Check available system resources (CPU, memory)
   - Ensure output path is writable (for file recording)

3. **Performance Issues**
   - Reduce encoder bitrate or resolution
   - Use hardware encoding if available
   - Close unnecessary applications

### Log Files
- OBS log files contain detailed plugin information
- Enable debug logging for detailed troubleshooting
- Submit log files with bug reports

## Contributing

We welcome contributions to the OBS TTOutput plugin! Here's how you can help:

### Development Setup
1. Fork the repository
2. Create a feature branch
3. Follow the coding standards (see `.clang-format`)
4. Write tests for new functionality
5. Submit a pull request

### Coding Standards
- Follow the existing code style
- Use meaningful variable and function names
- Add comments for complex logic
- Ensure cross-platform compatibility

### Reporting Issues
- Use the GitHub issue tracker
- Provide detailed reproduction steps
- Include OBS log files when relevant
- Specify your operating system and OBS version

## License

This project is licensed under the GPL v2 License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- **OBS Studio Team**: For the excellent streaming software and plugin API
- **Qt Project**: For the robust UI framework
- **Contributors**: All developers who have contributed to this project

## Support

- **Documentation**: Check this README and inline code comments
- **Issues**: Report bugs and feature requests on GitHub
- **Community**: Join the OBS Studio community forums for general support

---

**Note**: This plugin is not officially affiliated with OBS Studio. It is a community-developed extension designed to enhance OBS functionality.