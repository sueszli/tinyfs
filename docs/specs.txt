# C++ Developer Technical Assessment

This technical assessment evaluates your proficiency in modern C++ development, including network programming, build systems, testing practices, and code organization. You will build a simple HTTP file server using industry-standard libraries and tools.

## Task Description

Create an HTTP server application that serves static files from a configurable directory on the hosting computer:

1. **HTTP Server Implementation**
   - Use Boost.ASIO and/or Boost.Beast for HTTP server implementation
   - Handle concurrent requests appropriately
   - Serve files with correct MIME types and HTTP headers
   - Return appropriate HTTP status codes (200, 404, 500, etc.)
   - Nice to have: Show directory contents if a directory is requested
2. **Technical Requirements**
   - Use C++17 or later standard
   - Implement proper RAII and modern C++ best practices
   - Use smart pointers where appropriate
   - Implement proper error handling with exceptions or error codes
   - Support graceful shutdown on SIGINT/SIGTERM
   - Nice to have: Configurable server port via command line argument (default: 8080)
   - Nice to have: Configurable storage directory via command line argument (default: "./files")
3. **Build System Configuration**
   - Use CMake, Bazel, or equivalent modern build tools
   - Properly link Boost libraries
   - Support both Debug and Release builds
   - Include compiler warnings and treat warnings as errors
   - Nice to have: Support building on both Linux and macOS

## Deliverables

Your submission must include:

*  Source Code
*  Build Configuration (e.g. `CMakeLists.txt` with proper target definitions)
* `README.md` with build and run instructions
* Any additional configuration files needed

## Rationale

The goal of this exercise is to assess:

* How you approach technical problems and system design
* Your ability to structure and organize a complete project
* Your quality standards (testing, error handling, documentation)
* Your proficiency with modern C++ and industry-standard tools
* Your coding style and attention to detail

**Note**: Focus on demonstrating clean, production-quality code rather than advanced optimizations. We value thorough understanding, correctness, maintainability, and testing over premature optimization.
