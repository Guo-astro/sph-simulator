#!/usr/bin/env python3
"""
Create animated comparison of 2D Baseline simulation
"""

import argparse
from animate_2d import create_animation


def main():
    parser = argparse.ArgumentParser(description='Animate 2D Baseline')
    parser.add_argument('--baseline', default='../results/baseline',
                        help='Baseline output directory')
    parser.add_argument('--output', default='baseline_2d.mp4',
                        help='Output animation file')
    parser.add_argument('--gamma', type=float, default=1.4,
                        help='Adiabatic index')
    args = parser.parse_args()

    create_animation(args.baseline, args.output, args.gamma, 'Baseline')


if __name__ == '__main__':
    main()
