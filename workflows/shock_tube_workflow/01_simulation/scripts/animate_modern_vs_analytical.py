#!/usr/bin/env python3
"""
Create animated comparison of Modern (with ghosts) vs analytical Sod shock tube solution
"""

import argparse
from shock_tube_animation_utils import create_animation


def main():
    parser = argparse.ArgumentParser(description='Animate Modern vs Analytical')
    parser.add_argument('--modern', default='../results/modern',
                        help='Modern output directory')
    parser.add_argument('--output', default='modern_vs_analytical.mp4',
                        help='Output animation file')
    parser.add_argument('--gamma', type=float, default=1.4,
                        help='Adiabatic index')
    args = parser.parse_args()

    create_animation(args.modern, args.output, args.gamma, 'Modern (with ghosts)')


if __name__ == '__main__':
    main()
