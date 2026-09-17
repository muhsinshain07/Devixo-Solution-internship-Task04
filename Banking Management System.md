# Banking Management System

This project is a console-based Banking Management System written in C++17. It demonstrates object-oriented programming, exception handling, STL containers, searching, sorting, authentication, transaction processing, and persistent file storage.

## Main source file

The complete application is provided in `banking_management_system.cpp`. It stores account information and transaction history in `accounts.dat`.

## Compile

### Windows

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic banking_management_system.cpp -o banking_management_system.exe
```

### Linux or macOS

```bash
g++ -std=c++17 -Wall -Wextra -pedantic banking_management_system.cpp -o banking_management_system
```

## Run

### Windows PowerShell

```powershell
.\banking_management_system.exe
```

### Linux or macOS

```bash
./banking_management_system
```

## Features

The application supports account creation, account viewing, account searching, account updating, account deletion, deposits, withdrawals, transfers, balance checks, account authentication, account sorting, transaction history, and account statements.

Savings accounts enforce a minimum balance of 500.00. Current accounts support a configurable overdraft limit. All important operations validate input and throw meaningful custom exceptions when an operation is invalid.

## OOP design

`Account` is an abstract base class. `SavingsAccount` and `CurrentAccount` inherit from it and override account-specific behavior. The application uses private data members for encapsulation, virtual functions for polymorphism, constructors for object initialization, and `unique_ptr` objects for safe ownership of account instances.

## Data storage

The program writes records to `accounts.dat` in the current working directory. Do not delete this file if you want to preserve accounts between executions. The storage format is intended for this program and should not be edited manually.

## Supplementary files

- `project_report.txt` contains a short academic project report.
- `sample_input.txt` contains an example sequence of menu inputs for testing.
- `oop_concepts_demo.cpp` is a small educational program that separately demonstrates inheritance, encapsulation, polymorphism, constructors, and exception handling.

## Troubleshooting

If `g++` is not recognized, install a C++ compiler and add its executable directory to the system PATH. Run the compile command from the folder containing `banking_management_system.cpp`. If the program appears to lose data, check that it is being run from the same folder where `accounts.dat` was created.

## Reference

[1]: https://code.visualstudio.com/docs/languages/cpp "Visual Studio Code C++ documentation"
