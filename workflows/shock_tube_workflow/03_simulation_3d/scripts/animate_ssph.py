#!/usr/bin/env python3
"""
Create animated visualization of 3D SSPH simulation
"""

import argparse
from animate_3d_utils import create_animation_3d


def main():
    parser = argparse.ArgumentParser(description='Animate 3D SSPH')
    parser.add_argument('--ssph', default='../results/ssph',
                        help='SSPH output directory')
    parser.add_argument('--output', default='ssph_3d.mp4',
                        help='Output animation file')
    parser.add_argument('--gamma', type=float, default=1.4,
                        help='Adiabatic index')
    args = parser.parse_args()

    create_animation_3d(args.ssph, args.output, args.gamma, 'SSPH')


if __name__ == '__main__':
    main()
