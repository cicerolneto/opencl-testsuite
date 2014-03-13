/*
** Copyright (c) 2014 The Khronos Group Inc.
**
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and/or associated documentation files (the
** "Materials"), to deal in the Materials without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Materials, and to
** permit persons to whom the Materials are furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be included
** in all copies or substantial portions of the Materials.
**
** THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
*/

#include <stdlib.h>
#include <iostream>
#include <set>
#include <vector>

#ifdef __APPLE__
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

// TODO: since platform info about all devices etc. seems to be needed everywhere
//       always start by collecting complete device info and use that for printing
//       and finding device etc.

namespace
{
    typedef std::vector<cl_platform_id> platform_vector;
    typedef std::vector<cl_device_id> device_vector;
}

platform_vector getPlatformIDs()
{
    const platform_vector::size_type maxPlatforms = 10;
    platform_vector platformIds(maxPlatforms);

    cl_uint numOfPlatforms = 0;
    cl_int ret = clGetPlatformIDs(maxPlatforms, platformIds.data(), &numOfPlatforms);
    if (ret != CL_SUCCESS)
    {
        std::cerr << "Failed to get platform ids." << std::endl;
        numOfPlatforms = 0;
    }
    platformIds.resize(numOfPlatforms);
    return platformIds;
}

device_vector getDeviceIDs(cl_platform_id const& platformId)
{
    cl_uint num_devices = 0;
    if (CL_SUCCESS != clGetDeviceIDs(platformId, CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices))
    {
        std::cerr << "Failed to get number of devices." << std::endl;
        return device_vector();
    }

    device_vector devices(num_devices);
    if (CL_SUCCESS != clGetDeviceIDs(platformId, CL_DEVICE_TYPE_ALL, num_devices, devices.data(), NULL))
    {
        std::cerr << "clGetDeviceIDs failed." << std::endl;
        num_devices = 0;
    }
    devices.resize(num_devices);
    return devices;
}

device_vector getAllDeviceIDs()
{
    platform_vector platforms = getPlatformIDs();
    device_vector all_devices;

    for (unsigned i = 0; i < platforms.size(); i++) {
        device_vector devices = getDeviceIDs(platforms[i]);
        for (unsigned j = 0; j < devices.size(); j++) {
            all_devices.push_back(devices[j]);
        }
    }
    return all_devices;
}

std::string getPlatformName(cl_platform_id const& platformId) {
    std::string platName(1024, '\0');
    std::size_t platNameLen = 0U;
    clGetPlatformInfo(platformId, CL_PLATFORM_NAME, 1024, &platName[0], &platNameLen);
    platName.resize(platNameLen);
    return platName;
}

std::string getDeviceString(std::string platformName, cl_device_id const& deviceId) {
    std::string deviceName(1024, '\0');
    std::string deviceVersion(1024, '\0');
    std::string driverVersion(1024, '\0');
    std::string openCLCVersion(1024, '\0');

    clGetDeviceInfo(deviceId, CL_DEVICE_NAME, 1024, &deviceName[0], NULL);
    clGetDeviceInfo(deviceId, CL_DEVICE_VERSION, 1024, &deviceVersion[0], NULL);
    clGetDeviceInfo(deviceId, CL_DRIVER_VERSION, 1024, &driverVersion[0], NULL);
    clGetDeviceInfo(deviceId, CL_DEVICE_OPENCL_C_VERSION, 1024, &openCLCVersion[0], NULL);

    // c_str() -> string seems to get rid of invalid chars if e.g. \0 was included in string
    return "{\"id\":" + std::to_string((unsigned long)deviceId) + ",\"platformName\":\"" + 
        std::string(platformName.c_str()) + "\",\"deviceName\":\"" + 
        std::string(deviceName.c_str()) + "\",\"deviceVersion\":\"" + 
        std::string(deviceVersion.c_str()) + "\",\"driverVersion\":\"" + 
        std::string(driverVersion.c_str()) + "\",\"openCLCVersion\":\"" + 
        std::string(openCLCVersion.c_str()) + "\"}";
}

/**
 * Read kernel code from stdin.
 */
std::string readAllInput()
{
    // don't skip the whitespace while reading
    std::cin >> std::noskipws;
    // use stream iterators to copy the stream to a string
    std::istream_iterator<char> begin(std::cin);
    std::istream_iterator<char> end;
    return std::string(begin, end);
}

/**
 * Compile source code with given device.
 */
bool compileSource(std::string const& source, cl_device_id const& deviceId, bool debug)
{
    cl_int ret = CL_SUCCESS;

    // Create an OpenCL context
    cl_context context = clCreateContext(NULL, 1, &deviceId, NULL, NULL, &ret);
    if (debug) std::cerr << "Creating context.\n";
    if (ret != CL_SUCCESS)
    {
        // TODO: add constant to string function to print error code...
        std::cerr << "Error: Failed to create OpenCL context." << std::endl;
        return false;
    }

    // Create the program
    const char *buf = source.c_str();
    if (debug) std::cerr << "Creating program.\n";
    cl_program program = clCreateProgramWithSource(context, 1, (const char **)&buf, NULL, &ret);

    // Build the program
    if (debug) std::cerr << "Building program.\n";
    if (CL_SUCCESS != clBuildProgram(program, 1, &deviceId, NULL, NULL, NULL))
    {
        // TODO: add constant to string function to print error code...
        std::cerr << "Error: Failed to build program." << std::endl;
        return false;
    }

    bool cleanupOk = true;
    cleanupOk &= CL_SUCCESS == clReleaseContext(context);
    if (!cleanupOk) {
        std::cerr << "Error: Could not clean up context after build." << std::endl;
        return false;
    }

    return true;
}

/**
 * Writes JSON to stdout printing device information.
 */
bool printDeviceInfo() {
    platform_vector platforms = getPlatformIDs();
    std::vector<std::string> deviceNames;
    for (unsigned i = 0; i < platforms.size(); i++) {
        device_vector devices = getDeviceIDs(platforms[i]);
        std::string platformName = getPlatformName(platforms[i]);
        for (unsigned j = 0; j < devices.size(); j++) {
            deviceNames.push_back(getDeviceString(platformName, devices[j]));
        }
    }

    std::cout << "{\"deviceIdentifiers\":[" << std::endl;

    for (unsigned deviceIdx = 0; deviceIdx < deviceNames.size(); deviceIdx++) {
        std::cout << deviceNames[deviceIdx];
        if ( deviceIdx != (deviceNames.size()-1) ) {
            std::cout << ",";
        }
        std::cout << std::endl;
    }
    std::cout << "]}" << std::endl;

    return true;
}

bool compileWithDevice(std::string selectedDevice, bool debug) {
    if (debug) std::cerr << "Reading stdin:" << std::endl;
    std::string source = readAllInput();
    if (debug) std::cerr << source << std::endl;

    if (debug) std::cerr << "Fetch all devices..." << std::endl;
    device_vector all_devices = getAllDeviceIDs();

    // find correct device by id
    if (debug) std::cerr << "Finding correct device to compile..." << std::endl;
    for (unsigned long i = 0; i < all_devices.size(); i++) {
        if (std::to_string((unsigned long)all_devices[i]) == selectedDevice) {
            if (debug) std::cerr << "Found: " << getDeviceString("", all_devices[i]) << std::endl;
            return compileSource(source, all_devices[i], debug);
        }
    }

    std::cerr << "Error: Could not find device." << std::endl;
    return false;
}

bool runWithDevice(std::string selectedDevice) {
    std::cerr << "Running kernel not implemented yet." << std::endl;
    return false;
}


////////////////////////////////////////////////////////////////////////////
//
// Parse commandline arguments and main
//

// Options structure, this shuld be populated by cmd parser
// and used in main() where commands are interpreted
struct {
    std::string command;
    std::string device;
    bool debug;
} Options = { "none", "none", false };

std::string argv0 = "ocl-tester";
bool fail(std::string reason) {
    std::cerr << reason << std::endl;
    std::cerr << "Usage: " << argv0 << " <command> [OPTIONS] [< kernelcode.cl]" << std::endl << std::endl;
    std::cerr << argv0 << " list-devices" << std::endl;
    std::cerr << argv0 << " compile --device 16918272 < kernel.cl" << std::endl;
    std::cerr << argv0 << " run-kernel --device 16918272 < kernel.cl" << std::endl << std::endl;
    std::cerr << "Available options:" << std::endl;
    std::cerr << "--debug                  Print debug information." << std::endl;
    std::cerr << "--device <device_id>     OpenCL device id which will be used to compile test case." << std::endl 
              << "                         Ids are returned with list-devices command" << std::endl << std::endl;
    return false;
}

int parseCommandLine(int argc, char const* argv[]) {

    std::string deviceFlag = "--device";
    std::string debugFlag = "--debug";

    if (argc > 1) {
        Options.command = argv[1];
    }

    for (int i = 2; i < argc; ++i) {
        std::string current(argv[i]);

        // if device is given
        if (deviceFlag.compare(current) == 0 && i+1 < argc) {
            Options.device = std::string(argv[i+1]); i++;
        } else if (debugFlag.compare(current) == 0) {
            Options.debug = true;
        }
    }

    if (Options.command.compare("none") == 0) {
        return fail("Invalid Arguments: Command missing.");
    }

    return true;
}

int main(int argc, char const* argv[])
{
    if (!parseCommandLine(argc, argv)) {
        return EXIT_FAILURE;
    }

    bool success = true;
    if (Options.command.compare("list-devices") == 0) {
        success = printDeviceInfo();
    } else if (Options.command.compare("compile") == 0) {
        success = compileWithDevice(Options.device, Options.debug);
    } else if (Options.command.compare("run-kernel") == 0) {
        success = runWithDevice(Options.device);
    } else {
        fail("Invalid Arguments: Unknown command: " + Options.command);
        return EXIT_FAILURE;
    }
    
    if (success) {
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}
