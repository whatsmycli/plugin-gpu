// GPU Detection Plugin for whatsmycli
// Copyright (C) 2025 enXov
// License: GPLv3

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstring>
#include <algorithm>

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
    #include <windows.h>
    #include <setupapi.h>
    #include <devguid.h>
    #pragma comment(lib, "setupapi.lib")
#elif defined(__APPLE__)
    #define PLATFORM_MACOS
    #include <IOKit/IOKitLib.h>
#elif defined(__linux__)
    #define PLATFORM_LINUX
#endif

// Plugin API export macro
#ifdef _WIN32
    #define WHATSMY_PLUGIN_EXPORT __declspec(dllexport)
#else
    #define WHATSMY_PLUGIN_EXPORT
#endif

namespace fs = std::filesystem;

// GPU information structure
struct GPUInfo {
    std::string name;
    std::string vendor;
    std::string driver_version;
    std::string pci_id;
    int index;
    bool is_active;
};

// ANSI color codes
namespace Color {
    const char* RESET = "\033[0m";
    const char* BOLD = "\033[1m";
    const char* CYAN = "\033[36m";
    const char* GREEN = "\033[32m";
    const char* YELLOW = "\033[33m";
    const char* BLUE = "\033[34m";
    const char* DIM = "\033[2m";
}

// Helper function to print section headers
void print_header(const std::string& text) {
    std::cout << "\n" << Color::BOLD << Color::CYAN << text << Color::RESET << "\n";
    std::cout << std::string(50, '=') << "\n";
}

// Helper function to print key-value pairs
void print_field(const std::string& key, const std::string& value) {
    std::cout << "  " << Color::GREEN << key << ": " << Color::RESET << value << "\n";
}

// Get vendor name from PCI ID
std::string get_vendor_name(const std::string& vendor_id) {
    std::string id = vendor_id;
    // Convert to lowercase for comparison
    std::transform(id.begin(), id.end(), id.begin(), ::tolower);
    
    if (id == "0x10de" || id == "10de") return "NVIDIA";
    if (id == "0x1002" || id == "1002") return "AMD";
    if (id == "0x8086" || id == "8086") return "Intel";
    return "Unknown";
}

#ifdef PLATFORM_LINUX
// Linux GPU detection using /sys/class/drm
std::vector<GPUInfo> detect_gpus_linux() {
    std::vector<GPUInfo> gpus;
    const std::string drm_path = "/sys/class/drm";
    
    if (!fs::exists(drm_path)) {
        return gpus;
    }
    
    int gpu_index = 0;
    for (const auto& entry : fs::directory_iterator(drm_path)) {
        std::string card_name = entry.path().filename().string();
        
        // Only process card* entries (not card*-HDMI, card*-DP, etc.)
        if (card_name.find("card") == 0 && card_name.find("-") == std::string::npos) {
            GPUInfo gpu;
            gpu.index = gpu_index++;
            gpu.is_active = (gpu.index == 0); // First GPU is typically active
            
            // Read device info from uevent file
            std::string uevent_path = entry.path().string() + "/device/uevent";
            if (fs::exists(uevent_path)) {
                std::ifstream uevent(uevent_path);
                std::string line;
                std::string vendor_id, device_id;
                
                while (std::getline(uevent, line)) {
                    if (line.find("PCI_ID=") == 0) {
                        gpu.pci_id = line.substr(7);
                        // Split vendor:device
                        size_t colon = gpu.pci_id.find(':');
                        if (colon != std::string::npos) {
                            vendor_id = gpu.pci_id.substr(0, colon);
                            device_id = gpu.pci_id.substr(colon + 1);
                        }
                    } else if (line.find("PCI_SLOT_NAME=") == 0) {
                        // Could use this for more detailed info
                    }
                }
                
                gpu.vendor = get_vendor_name(vendor_id);
            }
            
            // Try to read GPU name from various sources
            std::vector<std::string> name_paths = {
                entry.path().string() + "/device/label",
                entry.path().string() + "/device/product_name",
                entry.path().string() + "/device/model"
            };
            
            bool name_found = false;
            for (const auto& path : name_paths) {
                if (fs::exists(path)) {
                    std::ifstream name_file(path);
                    std::string name;
                    if (std::getline(name_file, name) && !name.empty()) {
                        gpu.name = name;
                        name_found = true;
                        break;
                    }
                }
            }
            
            // If no name found, construct a basic one with PCI ID
            if (!name_found) {
                if (!gpu.pci_id.empty()) {
                    gpu.name = gpu.vendor + " GPU [" + gpu.pci_id + "]";
                } else {
                    gpu.name = gpu.vendor + " GPU";
                }
            }
            
            // Try to get NVIDIA driver version
            if (gpu.vendor == "NVIDIA") {
                std::string nvidia_version_path = "/proc/driver/nvidia/version";
                if (fs::exists(nvidia_version_path)) {
                    std::ifstream version_file(nvidia_version_path);
                    std::string line;
                    while (std::getline(version_file, line)) {
                        if (line.find("Kernel Module") != std::string::npos) {
                            // Extract version number after "Kernel Module"
                            size_t pos = line.find("Kernel Module");
                            if (pos != std::string::npos) {
                                std::string remainder = line.substr(pos + 13); // Skip "Kernel Module"
                                std::istringstream iss(remainder);
                                std::string version;
                                if (iss >> version) { // Read first token (the version)
                                    gpu.driver_version = version;
                                }
                            }
                        }
                    }
                }
            }
            
            gpus.push_back(gpu);
        }
    }
    
    return gpus;
}
#endif

#ifdef PLATFORM_WINDOWS
// Windows GPU detection using Setup API
std::vector<GPUInfo> detect_gpus_windows() {
    std::vector<GPUInfo> gpus;
    
    // Initialize device information set for display adapters
    HDEVINFO deviceInfoSet = SetupDiGetClassDevs(&GUID_DEVCLASS_DISPLAY, NULL, NULL, DIGCF_PRESENT);
    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        return gpus;
    }
    
    SP_DEVINFO_DATA deviceInfoData;
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    
    int gpu_index = 0;
    for (DWORD i = 0; SetupDiEnumDeviceInfo(deviceInfoSet, i, &deviceInfoData); i++) {
        GPUInfo gpu;
        gpu.index = gpu_index++;
        gpu.is_active = (gpu.index == 0);
        
        // Get device description (GPU name)
        char buffer[256];
        if (SetupDiGetDeviceRegistryPropertyA(deviceInfoSet, &deviceInfoData, SPDRP_DEVICEDESC,
                                              NULL, (PBYTE)buffer, sizeof(buffer), NULL)) {
            gpu.name = buffer;
        }
        
        // Get manufacturer (vendor)
        if (SetupDiGetDeviceRegistryPropertyA(deviceInfoSet, &deviceInfoData, SPDRP_MFG,
                                              NULL, (PBYTE)buffer, sizeof(buffer), NULL)) {
            gpu.vendor = buffer;
        }
        
        // Get driver version
        if (SetupDiGetDeviceRegistryPropertyA(deviceInfoSet, &deviceInfoData, SPDRP_DRIVER,
                                              NULL, (PBYTE)buffer, sizeof(buffer), NULL)) {
            gpu.driver_version = buffer;
        }
        
        // Get hardware ID for PCI information
        if (SetupDiGetDeviceRegistryPropertyA(deviceInfoSet, &deviceInfoData, SPDRP_HARDWAREID,
                                              NULL, (PBYTE)buffer, sizeof(buffer), NULL)) {
            std::string hwid = buffer;
            // Parse PCI\VEN_XXXX&DEV_XXXX format
            size_t ven_pos = hwid.find("VEN_");
            size_t dev_pos = hwid.find("DEV_");
            if (ven_pos != std::string::npos && dev_pos != std::string::npos) {
                std::string vendor = hwid.substr(ven_pos + 4, 4);
                std::string device = hwid.substr(dev_pos + 4, 4);
                gpu.pci_id = vendor + ":" + device;
            }
        }
        
        gpus.push_back(gpu);
    }
    
    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return gpus;
}
#endif

#ifdef PLATFORM_MACOS
// macOS GPU detection using IOKit (placeholder)
std::vector<GPUInfo> detect_gpus_macos() {
    std::vector<GPUInfo> gpus;
    
    // TODO: Implement IOKit-based GPU detection
    GPUInfo gpu;
    gpu.index = 0;
    gpu.is_active = true;
    gpu.name = "macOS GPU (detection not implemented)";
    gpu.vendor = "Unknown";
    gpu.driver_version = "N/A";
    gpu.pci_id = "N/A";
    gpus.push_back(gpu);
    
    return gpus;
}
#endif

// Cross-platform GPU detection
std::vector<GPUInfo> detect_gpus() {
#ifdef PLATFORM_LINUX
    return detect_gpus_linux();
#elif PLATFORM_WINDOWS
    return detect_gpus_windows();
#elif PLATFORM_MACOS
    return detect_gpus_macos();
#else
    return {};
#endif
}

// Display a single GPU
void display_gpu(const GPUInfo& gpu, bool brief = false) {
    if (brief) {
        std::cout << Color::BOLD << "GPU " << gpu.index << Color::RESET;
        if (gpu.is_active) {
            std::cout << " " << Color::GREEN << "(Active)" << Color::RESET;
        }
        std::cout << "\n";
        print_field("Name", gpu.name);
        print_field("Vendor", gpu.vendor);
        if (!gpu.pci_id.empty()) {
            print_field("PCI ID", gpu.pci_id);
        }
    } else {
        print_header("GPU " + std::to_string(gpu.index) + (gpu.is_active ? " (Active)" : ""));
        print_field("Name", gpu.name);
        print_field("Vendor", gpu.vendor);
        if (!gpu.driver_version.empty() && gpu.driver_version != "N/A") {
            print_field("Driver Version", gpu.driver_version);
        }
        if (!gpu.pci_id.empty() && gpu.pci_id != "N/A") {
            print_field("PCI ID", gpu.pci_id);
        }
    }
}

// Display all GPUs
void display_all_gpus(const std::vector<GPUInfo>& gpus) {
    if (gpus.empty()) {
        std::cout << Color::YELLOW << "No GPUs detected." << Color::RESET << "\n";
        return;
    }
    
    print_header("All GPUs (" + std::to_string(gpus.size()) + " detected)");
    for (const auto& gpu : gpus) {
        display_gpu(gpu, true);
        if (&gpu != &gpus.back()) {
            std::cout << "\n";
        }
    }
}

// Display help
void display_help() {
    std::cout << Color::BOLD << "GPU Plugin for whatsmycli" << Color::RESET << "\n\n";
    std::cout << "Usage:\n";
    std::cout << "  whatsmy gpu           " << Color::DIM << "# Show active/default GPU" << Color::RESET << "\n";
    std::cout << "  whatsmy gpu all       " << Color::DIM << "# Show all GPUs" << Color::RESET << "\n";
    std::cout << "  whatsmy gpu <index>   " << Color::DIM << "# Show specific GPU by index" << Color::RESET << "\n";
    std::cout << "  whatsmy gpu help      " << Color::DIM << "# Show this help" << Color::RESET << "\n";
}

// Plugin entry point (API v2)
extern "C" WHATSMY_PLUGIN_EXPORT int plugin_run(int argc, char* argv[]) {
    try {
        // Detect GPUs
        std::vector<GPUInfo> gpus = detect_gpus();
        
        if (gpus.empty()) {
            std::cerr << Color::YELLOW << "Warning: No GPUs detected." << Color::RESET << "\n";
            std::cerr << "This could mean:\n";
            std::cerr << "  - No GPU is present in the system\n";
            std::cerr << "  - GPU drivers are not installed\n";
            std::cerr << "  - Insufficient permissions to access GPU information\n";
            return 1;
        }
        
        // Parse arguments
        if (argc == 1) {
            // No arguments: show active GPU
            for (const auto& gpu : gpus) {
                if (gpu.is_active) {
                    display_gpu(gpu);
                    return 0;
                }
            }
            // If no active GPU, show first one
            display_gpu(gpus[0]);
            
        } else if (argc == 2) {
            std::string arg = argv[1];
            
            if (arg == "help" || arg == "--help" || arg == "-h") {
                display_help();
                return 0;
            } else if (arg == "all") {
                display_all_gpus(gpus);
                return 0;
            } else {
                // Try to parse as GPU index
                try {
                    int index = std::stoi(arg);
                    if (index < 0 || index >= static_cast<int>(gpus.size())) {
                        std::cerr << Color::YELLOW << "Error: GPU index " << index << " out of range." << Color::RESET << "\n";
                        std::cerr << "Available GPUs: 0-" << (gpus.size() - 1) << "\n";
                        return 1;
                    }
                    display_gpu(gpus[index]);
                    return 0;
                } catch (const std::exception&) {
                    std::cerr << Color::YELLOW << "Error: Invalid argument '" << arg << "'." << Color::RESET << "\n";
                    std::cerr << "Use 'whatsmy gpu help' for usage information.\n";
                    return 1;
                }
            }
        } else {
            std::cerr << Color::YELLOW << "Error: Too many arguments." << Color::RESET << "\n";
            std::cerr << "Use 'whatsmy gpu help' for usage information.\n";
            return 1;
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << Color::YELLOW << "Error: " << e.what() << Color::RESET << "\n";
        return 1;
    } catch (...) {
        std::cerr << Color::YELLOW << "Error: Unknown exception occurred." << Color::RESET << "\n";
        return 1;
    }
}

