#!/usr/bin/env python3
"""
Create animated comparison of 2D GSPH simulation
"""

import argparse
from animate_2d import create_animation


def main():
    parser = argparse.ArgumentParser(description='Animate 2D GSPH')
    parser.add_argument('--gsph', default='../results/gsph',
                        help='GSPH output directory')
    parser.add_argument('--output', default='gsph_2d.mp4',
                        help='Output animation file')
    parser.add_argument('--gamma', type=float, default=1.4,
                        help='Adiabatic index')
    args = parser.parse_args()

    create_animation(args.gsph, args.output, args.gamma, 'GSPH')


if __name__ == '__main__':
    main()
