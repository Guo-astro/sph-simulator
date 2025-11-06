#!/usr/bin/env python3
"""
Create animated comparison of SSPH vs analytical Sod shock tube solution
"""

import argparse
from shock_tube_animation_utils import create_animation


def main():
    parser = argparse.ArgumentParser(description='Animate SSPH vs Analytical')
    parser.add_argument('--ssph', default='../results/ssph',
                        help='SSPH output directory')
    parser.add_argument('--output', default='ssph_vs_analytical.mp4',
                        help='Output animation file')
    parser.add_argument('--gamma', type=float, default=1.4,
                        help='Adiabatic index')
    args = parser.parse_args()

    create_animation(args.ssph, args.output, args.gamma, 'SSPH')


if __name__ == '__main__':
    main()
