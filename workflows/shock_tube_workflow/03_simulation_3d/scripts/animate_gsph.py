#!/usr/bin/env python3
"""
Create animated visualization of 3D GSPH simulation
"""

import argparse
from animate_3d_utils import create_animation_3d


def main():
    parser = argparse.ArgumentParser(description='Animate 3D GSPH')
    parser.add_argument('--gsph', default='../results/gsph',
                        help='GSPH output directory')
    parser.add_argument('--output', default='gsph_3d.mp4',
                        help='Output animation file')
    parser.add_argument('--gamma', type=float, default=1.4,
                        help='Adiabatic index')
    args = parser.parse_args()

    create_animation_3d(args.gsph, args.output, args.gamma, 'GSPH')


if __name__ == '__main__':
    main()
