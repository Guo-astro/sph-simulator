# Code Quality Tools for SPH Simulation

This document describes the static analysis and code quality tools configured for this project.

## Tools

### 1. Clang-Tidy
Static analysis tool that checks for common C++ programming errors, style violations, and performance issues.

**Configuration:** `.clang-tidy`

**Usage:**
```bash
# Enable during CMake build
cmake -DENABLE_CLANG_TIDY=ON ..
make

# Run manually on specific files
clang-tidy include/core/*.hpp src/core/*.cpp -- -Iinclude -std=c++17

# Run on all source files
find include src -name '*.cpp' -o -name '*.hpp' | xargs clang-tidy -p build
```

**Enabled Check Categories:**
- `bugprone-*`: Detect common programming errors
- `cert-*`: CERT secure coding guidelines
- `clang-analyzer-*`: Deep static analysis
- `concurrency-*`: Thread safety issues
- `cppcoreguidelines-*`: C++ Core Guidelines compliance
- `google-*`: Google C++ style guide
- `modernize-*`: Modern C++ best practices
- `performance-*`: Performance optimizations
- `readability-*`: Code readability improvements

### 2. Clang-Format
Code formatting tool to ensure consistent style across the codebase.

**Configuration:** `.clang-format`

**Usage:**
```bash
# Format a single file
clang-format -i include/core/simulation.hpp

# Format all C++ files
find include src -name '*.cpp' -o -name '*.hpp' -o -name '*.tpp' | xargs clang-format -i

# Check formatting without modifying files
clang-format --dry-run --Werror include/core/*.hpp
```

**Style Highlights:**
- Based on LLVM style
- 4-space indentation
- 120 character line limit
- Pointer/reference alignment to the left
- Consistent brace placement

### 3. Compiler Warnings
Comprehensive compiler warnings enabled in CMakeLists.txt:

```cmake
-Wall          # Enable all common warnings
-Wextra        # Enable extra warnings
-Wno-sign-compare
-Wno-maybe-uninitialized
```

## Integration with Build System

### CMake Options
- `ENABLE_CLANG_TIDY=ON/OFF`: Enable/disable clang-tidy during build (default: OFF)
- `BUILD_TESTING=ON/OFF`: Enable/disable building tests (default: ON)

### Building with Quality Checks
```bash
# Configure with clang-tidy enabled
cd build
cmake -DENABLE_CLANG_TIDY=ON ..

# Build (will run clang-tidy on each file)
make -j8
```

## Workflow Integration

### Pre-Commit Checks
For automated checks before committing, you can create a git pre-commit hook:

```bash
#!/bin/bash
# .git/hooks/pre-commit

# Format check
echo "Checking code formatting..."
CHANGED_FILES=$(git diff --cached --name-only --diff-filter=ACM | grep -E '\.(cpp|hpp|tpp)$')

if [ -n "$CHANGED_FILES" ]; then
    for file in $CHANGED_FILES; do
        clang-format --dry-run --Werror "$file"
        if [ $? -ne 0 ]; then
            echo "❌ Format check failed for $file"
            echo "Run: clang-format -i $file"
            exit 1
        fi
    done
fi

echo "✅ All checks passed"
```

### Continuous Integration
For CI/CD pipelines, add these steps:

```yaml
# Example GitHub Actions workflow
- name: Install tools
  run: |
    sudo apt-get update
    sudo apt-get install -y clang-tidy clang-format

- name: Check formatting
  run: |
    find include src -name '*.cpp' -o -name '*.hpp' | \
    xargs clang-format --dry-run --Werror

- name: Run clang-tidy
  run: |
    cmake -DENABLE_CLANG_TIDY=ON -B build
    cmake --build build
```

## Best Practices

### Before Committing
1. **Format your code:**
   ```bash
   clang-format -i <modified-files>
   ```

2. **Run local checks:**
   ```bash
   make -j8  # If ENABLE_CLANG_TIDY=ON
   ```

3. **Fix warnings:** Address any clang-tidy warnings before committing

### During Development
1. Keep functions under 150 lines
2. Follow naming conventions (see `.clang-tidy`)
3. Use `const` wherever possible
4. Prefer modern C++17 features
5. Add comments for complex algorithms

### Suppressing False Positives
If clang-tidy reports a false positive, you can suppress it with a comment:

```cpp
// NOLINTNEXTLINE(check-name)
int risky_but_correct_code();
```

Or for a block:
```cpp
// NOLINTBEGIN(check-name)
void complex_function() {
    // ...
}
// NOLINTEND(check-name)
```

## Naming Conventions (Enforced by clang-tidy)

- **Namespaces:** `lower_case`
- **Classes/Structs:** `CamelCase`
- **Functions:** `lower_case`
- **Variables:** `lower_case`
- **Private/Protected Members:** `trailing_underscore_`
- **Constants/Enums:** `UPPER_CASE`
- **Macros:** `UPPER_CASE`

## Troubleshooting

### Clang-Tidy Too Slow
```bash
# Use compilation database
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..

# Run in parallel
run-clang-tidy -j8
```

### Too Many Warnings
Start by fixing critical categories:
1. `bugprone-*`
2. `cert-*`
3. `clang-analyzer-*`

Then gradually address others.

### Configuration Conflicts
The `.clang-tidy` file takes precedence over command-line options and built-in defaults.

## References

- [Clang-Tidy Documentation](https://clang.llvm.org/extra/clang-tidy/)
- [Clang-Format Documentation](https://clang.llvm.org/docs/ClangFormat.html)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [CERT C++ Coding Standard](https://wiki.sei.cmu.edu/confluence/pages/viewpage.action?pageId=88046682)
