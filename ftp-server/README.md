# FTP Server

This directory contains the implementation of an FTP (File Transfer Protocol) server written in C.
The project demonstrates medium-level concepts of the C programming language and explores networking, multithreading, and file handling through a practical implementation.

## 🌟 Features

- **FTP Server Implementation:** A minimal and functional server adhering to FTP principles.
- **Client Implementation:** Includes a CLI-based FTP client for testing and interaction.
- **Logging Mechanism:** Provides logging functionality to track server activity.
- **Utility Functions:** Contains helper functions to assist with common operations.
- **Modular Codebase:** Code is organized into multiple files for better readability and maintainability.
- **Test Suite:** Includes test scripts and files in the `test` directory.

## 📂 Directory Structure
```
ftp-server/ 
├── Makefile # Build configuration 
├── TODO.md # Pending tasks and notes 
├── cli_ftp_client.c # FTP client implementation 
├── ftp-server.c # Main FTP server implementation 
├── ftp.h # Header file with shared declarations 
├── log.c # Logging functionality 
├── utils.c # Utility functions 
└── test/ # Test cases and supporting files
```

## 🚀 Getting Started

### Prerequisites

To build and run the FTP server, you will need:

- A C compiler (e.g., GCC)
- Basic understanding of networking concepts
- An FTP client (optional, for testing)

### Build Instructions

1. Navigate to the `ftp-server` directory:

   ```bash
   cd ftp-server
   ```
2. Build the project using the provided Makefile:
   ```
   make
   ```
   - This will generate the server and client binaries.

### Running the Server
To start the server, run the following command:
 ```
./cli_ftp_client
```

## Development

### Key Files
- ftp-server.c: The main implementation of the FTP server.
- cli_ftp_client.c: A command-line client for testing the server.
- log.c: Provides a simple logging mechanism to track server events.
- utils.c: Includes reusable helper functions.
- ftp.h: Header file containing shared declarations.

## Contributing
- Contributions are welcome!

### To contribute:

- Fork the repository.
- Create a feature branch (git checkout -b feature-name).
- Commit your changes (git commit -m 'Add feature').
- Push the branch (git push origin feature-name).
- Open a pull request.

## TODO
- Refer to the TODO.md file for pending tasks and features.

## Testing
- Tests are located in the test directory. To run tests, navigate to the directory and follow the provided instructions (if any).

## License
- This project is licensed under the MIT License. See the main repository's LICENSE file for more details.
