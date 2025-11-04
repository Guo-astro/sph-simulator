#!/bin/bash

# Helper script for SPH workflow operations

set -e

WORKFLOWS_DIR="/Users/guo/sph-simulation/workflows"

show_help() {
    cat << EOF
SPH Workflow Helper Script

Usage: $0 <command> <workflow_name> [options]

Commands:
    build <name>           Build workflow plugin
    clean <name>           Clean build artifacts
    visualize <name>       Create animation from output
    compare               Compare DISPH vs GSPH (shock_tube only)
    list                  List available workflows
    status <name>         Show workflow status

Workflows:
    shock_tube           (1D) Sod shock tube
    evrard               (3D) Gravitational collapse
    khi                  (2D) Kelvin-Helmholtz instability
    gresho_chan_vortex   (2D) Vortex test
    hydrostatic          (2D) Hydrostatic equilibrium
    pairing_instability  (2D) Pairing instability

Examples:
    $0 build shock_tube
    $0 visualize khi
    $0 compare
    $0 list

EOF
}

list_workflows() {
    echo "Available workflows:"
    echo ""
    echo "  Name                    | Dim | Status"
    echo "  ----------------------- | --- | ------"
    
    for workflow in shock_tube evrard khi gresho_chan_vortex hydrostatic pairing_instability; do
        workflow_dir="${WORKFLOWS_DIR}/${workflow}_workflow"
        plugin_file="${workflow_dir}/01_simulation/lib/lib${workflow}_plugin.dylib"
        
        case "$workflow" in
            shock_tube) dim="1D" ;;
            evrard) dim="3D" ;;
            *) dim="2D" ;;
        esac
        
        if [ -f "$plugin_file" ]; then
            status="✅ Built"
        else
            status="❌ Not built"
        fi
        
        printf "  %-23s | %3s | %s\n" "$workflow" "$dim" "$status"
    done
    echo ""
}

build_workflow() {
    local name=$1
    local workflow_dir="${WORKFLOWS_DIR}/${name}_workflow/01_simulation"
    
    if [ ! -d "$workflow_dir" ]; then
        echo "Error: Workflow '$name' not found"
        exit 1
    fi
    
    echo "Building ${name} workflow..."
    cd "$workflow_dir"
    
    rm -rf build lib bin
    cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
    cmake --build build -j4
    
    echo ""
    echo "✅ Build complete!"
    echo "   Plugin: ${workflow_dir}/lib/lib${name}_plugin.dylib"
}

clean_workflow() {
    local name=$1
    local workflow_dir="${WORKFLOWS_DIR}/${name}_workflow/01_simulation"
    
    if [ ! -d "$workflow_dir" ]; then
        echo "Error: Workflow '$name' not found"
        exit 1
    fi
    
    echo "Cleaning ${name} workflow..."
    cd "$workflow_dir"
    rm -rf build lib bin
    echo "✅ Clean complete!"
}

visualize_workflow() {
    local name=$1
    local workflow_dir="${WORKFLOWS_DIR}/${name}_workflow"
    local output_dir="${workflow_dir}/02_output"
    
    if [ ! -d "$output_dir" ]; then
        echo "Error: No output directory found at $output_dir"
        echo "Run simulation first to generate output files"
        exit 1
    fi
    
    # Check if output directory has JSON files
    if [ -z "$(ls -A $output_dir/*.json 2>/dev/null)" ]; then
        echo "Error: No JSON output files found in $output_dir"
        exit 1
    fi
    
    # Determine dimension
    case "$name" in
        shock_tube) dim=1 ;;
        evrard) dim=3 ;;
        *) dim=2 ;;
    esac
    
    echo "Creating animation for ${name} (${dim}D)..."
    cd "$workflow_dir"
    
    if [ $dim -eq 1 ]; then
        python3 "${WORKFLOWS_DIR}/visualize.py" "$output_dir" --dim 1 --animate --output "${name}_animation.mp4"
    elif [ $dim -eq 2 ]; then
        python3 "${WORKFLOWS_DIR}/visualize.py" "$output_dir" --dim 2 --animate --quantity density --output "${name}_density.mp4"
        python3 "${WORKFLOWS_DIR}/visualize.py" "$output_dir" --dim 2 --animate --quantity pressure --output "${name}_pressure.mp4"
    else
        echo "3D visualization not yet implemented"
        exit 1
    fi
    
    echo "✅ Animation complete!"
}

compare_shock_tube() {
    local workflow_dir="${WORKFLOWS_DIR}/shock_tube_workflow"
    local disph_dir="${workflow_dir}/02_output/disph"
    local gsph_dir="${workflow_dir}/02_output/gsph"
    
    if [ ! -d "$disph_dir" ] || [ ! -d "$gsph_dir" ]; then
        echo "Error: Need both DISPH and GSPH output directories"
        echo "  DISPH: $disph_dir"
        echo "  GSPH:  $gsph_dir"
        exit 1
    fi
    
    echo "Comparing DISPH vs GSPH for shock tube..."
    cd "$workflow_dir"
    
    python3 compare_shock_tube.py --disph "$disph_dir" --gsph "$gsph_dir" --time 0.2
    
    echo "✅ Comparison complete!"
}

show_status() {
    local name=$1
    local workflow_dir="${WORKFLOWS_DIR}/${name}_workflow"
    
    if [ ! -d "$workflow_dir" ]; then
        echo "Error: Workflow '$name' not found"
        exit 1
    fi
    
    echo "Workflow: $name"
    echo "Path: $workflow_dir"
    echo ""
    
    # Check plugin
    plugin_file="${workflow_dir}/01_simulation/lib/lib${name}_plugin.dylib"
    if [ -f "$plugin_file" ]; then
        echo "✅ Plugin: Built"
        ls -lh "$plugin_file"
    else
        echo "❌ Plugin: Not built"
    fi
    echo ""
    
    # Check output
    output_dir="${workflow_dir}/02_output"
    if [ -d "$output_dir" ]; then
        output_count=$(ls -1 "$output_dir"/*.json 2>/dev/null | wc -l)
        echo "📊 Output: $output_count JSON files"
    else
        echo "📊 Output: No output directory"
    fi
    echo ""
    
    # Check config
    config_file="${workflow_dir}/config/production.json"
    if [ -f "$config_file" ]; then
        echo "⚙️  Config: production.json exists"
    fi
}

# Main command dispatcher
case "$1" in
    build)
        if [ -z "$2" ]; then
            echo "Error: Please specify workflow name"
            show_help
            exit 1
        fi
        build_workflow "$2"
        ;;
    clean)
        if [ -z "$2" ]; then
            echo "Error: Please specify workflow name"
            show_help
            exit 1
        fi
        clean_workflow "$2"
        ;;
    visualize)
        if [ -z "$2" ]; then
            echo "Error: Please specify workflow name"
            show_help
            exit 1
        fi
        visualize_workflow "$2"
        ;;
    compare)
        compare_shock_tube
        ;;
    list)
        list_workflows
        ;;
    status)
        if [ -z "$2" ]; then
            echo "Error: Please specify workflow name"
            show_help
            exit 1
        fi
        show_status "$2"
        ;;
    help|--help|-h)
        show_help
        ;;
    *)
        echo "Error: Unknown command '$1'"
        echo ""
        show_help
        exit 1
        ;;
esac
