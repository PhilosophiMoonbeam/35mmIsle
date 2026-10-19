#!/usr/bin/env python3
"""Materialize the actual pinned Cave lifecycle21 Source definitions privately."""
import argparse
from pathlib import Path
from prepare_cave_actor import prepare_provider

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository",required=True,type=Path)
    parser.add_argument("--manifest",required=True,type=Path)
    parser.add_argument("--output",required=True,type=Path)
    parser.add_argument("--evidence",required=True,type=Path)
    args=parser.parse_args()
    prepare_provider("cave-lifecycle",args.repository,args.manifest,args.output,args.evidence,Path(__file__))

if __name__=="__main__":
    main()
