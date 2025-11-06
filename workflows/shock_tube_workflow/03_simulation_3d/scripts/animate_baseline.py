#!/usr/bin/env python3
"""
Create animated visualization of 3D Baseline simulation
"""

import argparse
from animate_3d_utils import create_animation_3d


def main():
    parser = argparse.ArgumentParser(description='Animate 3D Baseline')
    parser.add_argument('--baseline', default='../results/baseline',
                        help='Baseline output directory')
    parser.add_argument('--output', default='baseline_3d.mp4',
                        help='Output animation file')
    parser.add_argument('--gamma', type=float, default=1.4,
                        help='Adiabatic index')
    args = parser.parse_args()

    create_animation_3d(args.baseline, args.output, args.gamma, 'Baseline')


if __name__ == '__main__':
    main()
