#!/usr/bin/env python3
"""
Create animated comparison of 2D DISPH simulation
"""

import argparse
from animate_2d import create_animation


def main():
    parser = argparse.ArgumentParser(description='Animate 2D DISPH')
    parser.add_argument('--disph', default='../results/disph',
                        help='DISPH output directory')
    parser.add_argument('--output', default='disph_2d.mp4',
                        help='Output animation file')
    parser.add_argument('--gamma', type=float, default=1.4,
                        help='Adiabatic index')
    args = parser.parse_args()

    create_animation(args.disph, args.output, args.gamma, 'DISPH')


if __name__ == '__main__':
    main()
