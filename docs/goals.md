I want to learn modern C++ development, including network programming, build systems, testing practices and code organization.

This is an HTTP server application (file server) using industry-standard libraries and tools that serves static files from a configurable directory on the hosting computer.

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
4. **Code Quality**
   - Structure project, organize code
   - Assure quality standards are met: testing, error handling, documentation, code style, attention to detail
   - Demonstrate clean, production-quality code rather than advanced optimizations. Value correctness, maintainability and testing over premature optimization.
