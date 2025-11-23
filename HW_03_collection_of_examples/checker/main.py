#!/usr/bin/env python3

import argparse, sys
from checker import Checker, ModuleMap

parser = argparse.ArgumentParser(description='Проверка параметров модуля ядра.')
parser.add_argument('test_name', type=str, help='Имя теста')
args = parser.parse_args()

if not args.test_name in ModuleMap :
    print(f"Unknown test name '{args.test_name}'")
    sys.exit(2)

checker = Checker([args.test_name, ModuleMap[args.test_name]])

print("Проверка hash:", checker.check_hash())
