#!/usr/bin/env python3
"""
Create animated comparison of 2D Modern simulation
"""

import argparse
from animate_2d import create_animation


def main():
    parser = argparse.ArgumentParser(description='Animate 2D Modern')
    parser.add_argument('--modern', default='../results/modern',
                        help='Modern output directory')
    parser.add_argument('--output', default='modern_2d.mp4',
                        help='Output animation file')
    parser.add_argument('--gamma', type=float, default=1.4,
                        help='Adiabatic index')
    args = parser.parse_args()

    create_animation(args.modern, args.output, args.gamma, 'Modern')


if __name__ == '__main__':
    main()
