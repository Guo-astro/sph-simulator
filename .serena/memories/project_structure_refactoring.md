# Project Structure Refactoring

## Date: 2025-11-04

## Changes Made

### 1. Documentation Consolidation
All documentation has been moved to the `docs/` folder:
- **Main docs**: BUILD.md, CONAN_MIGRATION.md, IMPLEMENTATION_SUMMARY.md, MIGRATION_GUIDE.md, QUICKSTART.md, plugin_architecture.md, create_workflow.sh
- **Archive**: Intermediate docs moved to `docs/archive/` (COMMIT_MESSAGE.txt, MIGRATION_COMPLETE.md)
- **New**: Created `docs/README.md` as documentation hub

### 2. Absolute Path Removal
Updated CMakePresets.json to use relative paths:
- Changed `"binaryDir": "/Users/guo/sph-simulation"` to `"binaryDir": "${sourceDir}/build"`
- Changed `"toolchainFile": "conan_toolchain.cmake"` to `"toolchainFile": "${sourceDir}/build/conan_toolchain.cmake"`

### 3. Updated Main README.md
- Added modern project overview with quick start guide
- Added links to documentation in docs/ folder
- Improved structure and readability
- Maintained Japanese legacy content

### 4. .gitignore
Already properly configured to ignore:
- Build artifacts (build/, CMakeFiles/, etc.)
- User-specific files (CMakeUserPresets.json)
- Conan generated files
- IDE and system files

## Current Structure

```
sph-simulation/
├── docs/                    # All documentation
│   ├── README.md           # Documentation hub
│   ├── BUILD.md
│   ├── QUICKSTART.md
│   ├── MIGRATION_GUIDE.md
│   ├── CONAN_MIGRATION.md
│   ├── IMPLEMENTATION_SUMMARY.md
│   ├── plugin_architecture.md
│   ├── create_workflow.sh
│   └── archive/            # Intermediate docs
│       ├── COMMIT_MESSAGE.txt
│       └── MIGRATION_COMPLETE.md
├── include/                # Public headers
├── src/                    # Core implementation
├── tests/                  # Unit tests
├── workflows/              # Plugin-based simulations
├── build/                  # Build artifacts (gitignored)
├── CMakeLists.txt
├── CMakePresets.json       # Now uses relative paths
├── conanfile.txt
└── README.md               # Updated main readme

```

## Benefits

1. **Cleaner root directory**: Only essential config files at root
2. **Portable**: No absolute paths in CMake configuration
3. **Organized**: All docs in one place with clear hierarchy
4. **Maintainable**: Clear separation between current and archived docs
5. **Professional**: Standard project structure

## Next Steps

Users should:
1. Review the changes with `git status`
2. Test build with the new CMakePresets.json configuration
3. Commit changes if everything works correctly
