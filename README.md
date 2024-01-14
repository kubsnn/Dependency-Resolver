# Dependency Resolver

[![ubuntu-tests](https://github.com/kubsnn/Dependency-Resolver/actions/workflows/ubuntu-ci.yml/badge.svg)](https://github.com/kubsnn/Dependency-Resolver/actions/workflows/ubuntu-ci.yml)
[![windows-tests](https://github.com/kubsnn/Dependency-Resolver/actions/workflows/windows-ci.yml/badge.svg)](https://github.com/kubsnn/Dependency-Resolver/actions/workflows/windows-ci.yml)

Dependency Resolver is a lightweight, header-only C++ library designed to simplify dependency injection and management in C++ applications. It is platform-independent and has been tested on both Ubuntu and Windows.

## Compatibility

Dependency Resolver is compatible with C++14 and higher. This ensures a wide range of modern C++ features can be utilized while maintaining compatibility with most current systems.

## Features

- Easy integration with C++ projects.
- Support for singleton, transient, and scoped lifetimes.
- Type-safe resolution of dependencies.
- Header-only library: no need to compile or link against.

## Installation

To use Dependency Resolver in your project, simply include the header file in your project:

```cpp
#include "dependency_resolver.hpp"
```

## Basic usage

Here's a simple example of how to use Dependency Resolver:

```cpp
#include <dependency_resolver.hpp>

using jaszyk::dependency_resolver;

class MyService {
public:
    void doSomething() {
        // Implementation here
    }
};

class MyController {
    std::shared_ptr<MyService> service;

public:
    MyController(std::shared_ptr<MyService> svc) : service(svc) {}

    void action() {
        service->doSomething();
    }
};

int main() {
    dependency_resolver resolver;

    // Register dependencies
    resolver.add_singleton<MyService>();

    // Resolve dependencies
    auto controller = resolver.resolve<MyController>();

    // Use the resolved object
    controller->action();

    return 0;
}
```

This example demonstrates registering a singleton service and resolving a controller that depends on that service.

## Advanced Usage

In this example, we will see how to use scoped and transient services with the Dependency Resolver. Scoped services are particularly useful for maintaining a single instance within a specific scope, while transient services create a new instance each time they are resolved.

### Example with Scoped and Transient Services

```cpp
#include <dependency_resolver.hpp>
#include <iostream>

using jaszyk::dependency_resolver;

class IDatabaseConnection {
public:
    virtual void connect() = 0;
};

class DatabaseConnection : public IDatabaseConnection {
public:
    void connect() {
        std::cout << "Database connection established." << std::endl;
    }
};

class Logger {
public:
    void log(const std::string& message) {
        std::cout << "Log: " << message << std::endl;
    }
};

class Application {
    std::shared_ptr<IDatabaseConnection> dbConnection;
    std::shared_ptr<Logger> logger;

public:
    Application(std::shared_ptr<IDatabaseConnection> dbConn, std::shared_ptr<Logger> log)
        : dbConnection(dbConn), logger(log) {}

    void run() {
        dbConnection->connect();
        logger->log("Application is running.");
    }
};

int main() {
    dependency_resolver resolver;

    // Registering a scoped DatabaseConnection service
    resolver.add_scoped<IDatabaseConnection, DatabaseConnection>();

    // Registering a transient Logger service
    resolver.add_transient<Logger>();

    // Creating a scope
    auto scope = resolver.make_scope();
    // Alternatively:
    // dependency_resolver::scope scope;

    // Resolving dependencies within the scope
    auto app = resolver.resolve<Application>(scope);

    // Alternative way of using temporary scope:
    // auto app = resolver.resolve<Application>(dependency_resolver::temporary_scope);

    // Running the application
    app->run();

    return 0;
}
```

## Contributing

Contributions are welcome! If you'd like to contribute, please fork the repository and use a feature branch. Pull requests are warmly welcome.
