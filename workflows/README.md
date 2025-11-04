# SPH Simulation Workflows

This directory contains complete simulation workflows following the plugin architecture.

## Quick Start

### Create a new workflow

```bash
./create_workflow.sh my_simulation collision --dim=3 --sph-type=gdisph
```

### Available workflow types

- `collision` - Cloud-cloud collision
- `merger` - Binary merger simulation
- `disk` - Disk relaxation
- `explosion` - Blast wave (Sedov-Taylor)
- `shock` - Shock tube test
- `custom` - Custom simulation

## Workflow Structure

Each workflow follows this structure:

```
workflow_name_workflow/
├── 01_simulation/
│   ├── src/
│   │   └── plugin.cpp           # Plugin implementation
│   ├── config/
│   │   ├── production.json      # Full simulation config
│   │   └── test.json           # Quick test config
│   ├── build/                   # Build artifacts (gitignored)
│   ├── output/                  # Simulation outputs (gitignored)
│   ├── results/                 # Post-processed results
│   ├── scripts/                 # Analysis scripts
│   └── docs/                    # Documentation
├── shared_data/                 # Data shared between steps
├── workflow_results/            # Aggregated results
└── workflow_logs/              # Execution logs
```

## Running Simulations

### Build the plugin

```bash
cd workflow_name_workflow/01_simulation
cmake -B build -S .
cmake --build build
```

### Run simulation

```bash
# From project root
./build/sph build/lib[plugin]_plugin.dylib workflow/.../config/test.json

# Or using make
cd workflow_name_workflow
make run-test
```

## Plugin Development

See `/docs/plugin_architecture.md` for detailed documentation on:
- Plugin interface
- Initial condition setup
- Configuration system
- Best practices

## Examples

### 1D Shock Tube
```bash
./create_workflow.sh shock_tube shock --dim=1 --sph-type=gsph
```

### 3D Binary Merger
```bash
./create_workflow.sh binary_merger merger --dim=3 --sph-type=gdisph
```

### Multi-step Workflow
```bash
./create_workflow.sh complex_sim custom --multi-step
```

## Testing

Run workflow tests:
```bash
cd workflow_name_workflow/01_simulation
cmake -B build -S . -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build
```

## Configuration Files

Workflows support JSON configuration with:
- **Inheritance**: Use `"extends": "base.json"` to reuse configs
- **Parameter overrides**: Override specific values per simulation
- **Environment variables**: Use `${VAR_NAME}` in configs

Example:
```json
{
  "extends": "../shared_data/base_config.json",
  "endTime": 10.0,
  "outputDirectory": "${WORKFLOW_DIR}/output"
}
```

## Best Practices

1. **Version Control**: Commit plugin source and configs, not outputs
2. **Documentation**: Update README.md with physics and expected results
3. **Testing**: Always create test config for quick validation
4. **Analysis**: Keep analysis scripts with the workflow
5. **Reproducibility**: Use `get_source_files()` to archive code state
