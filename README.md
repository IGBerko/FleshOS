# FleshOS

A Unix-like operating system for ESP32/ESP8266 microcontrollers with a complete file system interface and terminal commands.

![FleshOS Version](https://img.shields.io/badge/version-1.0-blue)
![Platform](https://img.shields.io/badge/platform-ESP32%20%7C%20ESP8266-green)
![License](https://img.shields.io/badge/license-IPL-orange)

## Overview

FleshOS is a lightweight operating system that brings Unix-like command-line functionality to ESP32 and ESP8266 microcontrollers. Built on top of LittleFS, it provides a familiar terminal interface with file system operations, directory navigation, and system management commands.

## Features

- **Unix-like Commands** - Familiar command-line interface with ls, cat, cd, pwd, mkdir, rm, and more
- **File System Operations** - Full support for file creation, editing, copying, moving, and deletion
- **Directory Navigation** - Complete directory structure with relative and absolute path support
- **System Protection** - Kernel panic mechanism to protect critical system files
- **Serial Interface** - Easy interaction through serial monitor at 115200 baud
- **LED Indicators** - Visual feedback for system status and errors
- **Lightweight** - Optimized for microcontroller constraints

## Supported Commands

### File Operations
- `ls [dir]` - List files and directories
- `cat <file>` - Display file contents
- `touch <file>` - Create empty file
- `write <file> <text>` - Create/overwrite file with text
- `append <file> <text>` - Append text to existing file
- `rm <file>` - Delete file
- `cp <src> <dest>` - Copy file
- `mv <src> <dest>` - Move/rename file or directory

### Directory Operations
- `pwd` - Print working directory
- `cd [dir]` - Change directory
- `mkdir <dir>` - Create directory
- `rmdir <dir>` - Remove empty directory

### System Commands
- `df` - Show filesystem usage statistics
- `ping <addr>` - Ping address (simulated)
- `reboot` - Restart the ESP device
- `help` - Display all available commands

## Installation

### Requirements
- ESP32 or ESP8266 board
- Arduino IDE or PlatformIO
- LittleFS library

### Setup

1. Clone this repository:
\`\`\`bash
git clone https://github.com/IGBerko/FleshOS/sketch-esp32-fleshos.ino
cd fleshos
\`\`\`

2. Open the project in Arduino IDE or PlatformIO

3. Install required libraries:
   - LittleFS (included with ESP32/ESP8266 core)

4. Configure your board:
   - Select your ESP32/ESP8266 board from Tools > Board
   - Set upload speed to 115200

5. Upload the code to your device

6. Open Serial Monitor at 115200 baud

## Usage

After uploading, open the Serial Monitor and you'll see:

\`\`\`
=== FleshOS Boot ===
----------------------------------------
Type 'help' to see available commands
----------------------------------------
FleshOS /> 
\`\`\`

### Example Session

\`\`\`bash
FleshOS /> mkdir projects
Directory created

FleshOS /> cd projects
FleshOS /projects/> touch readme.txt
File created

FleshOS /projects/> write readme.txt "Hello from FleshOS!"
File written

FleshOS /projects/> cat readme.txt
Hello from FleshOS!

FleshOS /projects/> ls
readme.txt (21 bytes)

FleshOS /projects/> cd ..
FleshOS /> pwd
/
\`\`\`

### Working with Paths

FleshOS supports both absolute and relative paths:

\`\`\`bash
# Absolute path
FleshOS /> cat /projects/readme.txt

# Relative path
FleshOS /projects/> cat readme.txt

# Parent directory
FleshOS /projects/> cd ..

# Current directory
FleshOS /> cd ./projects
\`\`\`

### Quotes for Spaces

Use quotes when working with text containing spaces:

\`\`\`bash
FleshOS /> write note.txt "This is a multi-word text"
FleshOS /> append note.txt " with more content"
\`\`\`

## Architecture

### Core Components

- **Command Parser** - Handles command parsing with quote support for arguments
- **Path Resolver** - Resolves relative and absolute paths with `.` and `..` support
- **File System Manager** - Interfaces with LittleFS for all storage operations
- **Kernel Panic System** - Protects critical system files from deletion

### Critical Files

FleshOS maintains two critical system files:
- `/sys.kernel` - Kernel version information
- `/bootmode.txt` - Boot mode configuration (contains easter egg)

Attempting to delete these files will trigger a kernel panic with LED indication.

### LED Indicators

- **Normal Operation** - Slow blinking (100ms on/off)
- **Boot Sequence** - 3 quick blinks
- **Kernel Panic** - Fast blinking (250ms on/off)

## Technical Details

- **Baud Rate**: 115200
- **File System**: LittleFS
- **LED Pin**: GPIO 2 (configurable)
- **Max Arguments**: 10 per command
- **Path Support**: Absolute and relative paths with `.` and `..`

## Development

### Adding New Commands

To add a new command, modify the `fleshosLoop()` function:

\`\`\`cpp
else if (command == "yourcommand") {
  if (argCount < 2) {
    Serial.println("Usage: yourcommand <arg>");
  } else {
    // Your command implementation
    Serial.println("Command executed");
  }
}
\`\`\`

Don't forget to add it to the help text!

### Customization

You can customize:
- LED pins in the defines at the top
- Maximum arguments in `parseArgs()`
- Critical files in `checkCriticalFiles()`
- Boot messages in `setup()`

## Troubleshooting

### Filesystem Mount Failed
If you see "FS mount failed", the system will attempt to format the filesystem. This is normal on first boot.

### Kernel Panic
If the device enters kernel panic mode (fast LED blinking):
1. Check serial output for the panic reason
2. Re-upload the code to restore critical files
3. Avoid deleting `/sys.kernel` or `/bootmode.txt`

### Commands Not Working
- Ensure Serial Monitor is set to 115200 baud
- Check that line ending is set to "Newline" or "Both NL & CR"
- Verify the command syntax with `help`

## Easter Eggs

Try running `cat /bootmode.txt` for a special message!

## License

MIT License - feel free to use and modify for your projects.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Author

Created by Itry

## Acknowledgments

- Built with Arduino framework
- Uses LittleFS for reliable flash storage
- Inspired by Unix/Linux command-line interfaces

---

**FleshOS v2.1** - Bringing Unix to microcontrollers
