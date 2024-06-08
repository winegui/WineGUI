# Contributing to WineGUI

First off, thank you for considering contributing to WineGUI! Your contributions are highly valued and help make this project better for everyone.

## Table of Contents

1. [Code of Conduct](#code-of-conduct)
2. [Getting Started](#getting-started)
3. [How to Contribute](#how-to-contribute)
4. [Coding Standards](#coding-standards)
5. [Commit Guidelines](#commit-guidelines)
6. [Pull Request Process](#pull-request-process)
7. [Resources](#resources)

## Code of Conduct

This project and everyone participating in it is governed by the [WineGUI Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are expected to uphold this code. Please report unacceptable behavior to: melroy at melroy dot org.

## Getting Started

### Forking the Repository

1. Fork the project repository from [GitHub](https://github.com/winegui/WineGUI). Or fork the [GitLab project](https://gitlab.melroy.org/melroy/winegui), if you have access to my [gitlab.melroy.org](https://gitlab.melroy.org) instance.
2. Clone your fork:
    ```sh
    git clone git@github.com:winegui/WineGUI.git
    cd WineGUI
    ```

### Setting Up Your Local Environment

1. Ensure you have all the necessary dependencies installed. Refer to the [README.md](README.md) for detailed installation instructions.
2. Create a new branch for your work:
    ```sh
    git checkout -b feature/my-feature
    ```

## How to Contribute

### Reporting Bugs

- Ensure the bug was not already reported by searching on the [GitHub issues](https://github.com/winegui/WineGUI/issues).
- If you're unable to find an open issue addressing the problem, [open a new one](https://github.com/winegui/WineGUI/issues/new). Be sure to include:
  - A descriptive title.
  - A clear description of the issue.
  - Steps to reproduce the problem.
  - Any relevant logs or screenshots (_Hint:_ running `winegui` command via terminal might output some additional information to the console).

### Suggesting Enhancements

- Check the [GitHub issues list](https://github.com/winegui/WineGUI/issues) for similar suggestions before opening a new one.
- Check the [**GitLab** issue list](https://gitlab.melroy.org/melroy/winegui/-/issues) as well before opening a new one.
- If you find none, you can [create a new issue](https://github.com/winegui/WineGUI/issues/new) that describes:
  - The enhancement you are suggesting.
  - Why you believe this enhancement would be beneficial.
  - Any relevant examples.

### Submitting Code Changes

1. Ensure your work aligns with the [Coding Standards](#coding-standards).
2. Ensure your commits follow the [Commit Guidelines](#commit-guidelines).
3. Push your branch to your forked repository on GitHub.
4. Open a Pull Request on the [GitHub repository](https://github.com/winegui/WineGUI/pulls).

Alternatively, if you prefer, you can submit Merge Requests via the [GitLab instance](https://gitlab.melroy.org/WineGUI/WineGUI) (assuming you have access).

## Coding Standards

- Follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html) as the basis.
- Additionally, adhere to the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines) as much as possible.
- Ensure your code is formatted using `clang-format` to maintain [consistent code style](.clang-format). Please run the following command to format your code before submitting:
  ```sh
  ./scripts/fix_format.sh
  ```

## Commit Guidelines

- Write clear, concise commit messages.
- Separate each change into its own commit.

## Pull Request Process

1. Ensure your changes pass all tests.
2. Ensure your code adheres to the coding standards and is properly formatted.
3. Create a descriptive title and include relevant details in the Pull Request.
4. Link any relevant issues (e.g., `Fixes #123`, `Closes #456`).
5. Request a review from the maintainers.
6. Address any feedback received during the review process promptly.

## Resources

- [GitHub Mirror](https://github.com/winegui/WineGUI)
- [GitLab Main Project](https://gitlab.melroy.org/WineGUI/WineGUI)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [Clang-Format Documentation](https://clang.llvm.org/docs/ClangFormat.html)
