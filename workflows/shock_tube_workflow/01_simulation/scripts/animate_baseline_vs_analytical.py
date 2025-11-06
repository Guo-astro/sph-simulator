#!/usr/bin/env python3
"""
Create animated comparison of Baseline (abd7353) vs analytical Sod shock tube solution
"""

import argparse
from shock_tube_animation_utils import create_animation


def main():
    parser = argparse.ArgumentParser(description='Animate Baseline vs Analytical')
    parser.add_argument('--baseline', default='../results/baseline',
                        help='Baseline output directory')
    parser.add_argument('--output', default='baseline_vs_analytical.mp4',
                        help='Output animation file')
    parser.add_argument('--gamma', type=float, default=1.4,
                        help='Adiabatic index')
    args = parser.parse_args()

    create_animation(args.baseline, args.output, args.gamma, 'Baseline (abd7353)')


if __name__ == '__main__':
    main()
