#!/usr/bin/env python3
"""
Create animated comparison of 2D SSPH simulation
"""

import argparse
from animate_2d import create_animation


def main():
    parser = argparse.ArgumentParser(description='Animate 2D SSPH')
    parser.add_argument('--ssph', default='../results/ssph',
                        help='SSPH output directory')
    parser.add_argument('--output', default='ssph_2d.mp4',
                        help='Output animation file')
    parser.add_argument('--gamma', type=float, default=1.4,
                        help='Adiabatic index')
    args = parser.parse_args()

    create_animation(args.ssph, args.output, args.gamma, 'SSPH')


if __name__ == '__main__':
    main()
