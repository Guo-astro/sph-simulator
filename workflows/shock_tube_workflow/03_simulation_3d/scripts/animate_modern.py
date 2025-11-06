#!/usr/bin/env python3
"""
Create animated visualization of 3D Modern simulation
"""

import argparse
from animate_3d_utils import create_animation_3d


def main():
    parser = argparse.ArgumentParser(description='Animate 3D Modern')
    parser.add_argument('--modern', default='../results/modern',
                        help='Modern output directory')
    parser.add_argument('--output', default='modern_3d.mp4',
                        help='Output animation file')
    parser.add_argument('--gamma', type=float, default=1.4,
                        help='Adiabatic index')
    args = parser.parse_args()

    create_animation_3d(args.modern, args.output, args.gamma, 'Modern')


if __name__ == '__main__':
    main()
