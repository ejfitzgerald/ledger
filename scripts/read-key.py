#!/usr/bin/env python3
import argparse
import os
import sys
from fetchai.ledger.crypto import Entity, Address


def parse_commandline():
    parser = argparse.ArgumentParser()
    parser.add_argument('key_file', help='The path to the keyfile to read')
    parser.add_argument('--public-key', action='store_true', help='Display the public key as well')
    parser.add_argument('--private-key', action='store_true', help='Display the private key as well')
    return parser.parse_args()


def main():
    args = parse_commandline()

    # check that the file exists
    if not os.path.exists(args.key_file):
        print('Unable to locate file: {}, check that is exists'.format(args.key_file))
        sys.exit(1)

    # attmept to load the key file
    with open(args.key_file, 'rb') as key_file:

        # reload the key and address information
        e = Entity(key_file.read())
        a = Address(e)

        if args.public_key or args.private_key:
            print('Address    : {}'.format(str(a)))
        else:
            print(str(a)) # in the default case don't bother with the prefix

        if args.public_key:
            print('Public Key : {}'.format(e.public_key))

        if args.private_key:
            print('Private Key: {}'.format(e.private_key))


if __name__ == '__main__':
    main()
