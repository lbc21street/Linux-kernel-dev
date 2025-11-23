from executor import ListExecutor, QueueExecutor, RbtreeExecutor
import sys
import random

def calc_hash(list) :
    hash = 0
    index = 0
    for value in list :
        hash ^= int(value + 7 * index)
        index += 1
    hash = hash ^ (len(list) << 16 | len(list))
    return hash

def generate_values(num, tail=False) :
    list = []
    i = 0
    for i in range(num) :
        if tail :
            list.append(random.randint(1, 0xFFFFFFFF))
        else :
            list.insert(0, random.randint(1, 0xFFFFFFFF))
    return list

def rotate_list_left(list, num) :
    size = len(list)
    if (size == 0) :
        return
    num = num % size
    return list[num:] + list[:num]


class ListTests:
    def __init__(self, ops):
        self.exec = ListExecutor(ops)

    def hash_tests(self):
        valList = generate_values(5)
        valListTail = generate_values(5, True)
        commonValList = valList + valListTail
        hash = calc_hash(commonValList)
        hashTail = calc_hash(commonValList[::-1])
        rotateNum = 3
        rotatedValList = rotate_list_left(commonValList, rotateNum)
        rotatedHash = calc_hash(rotatedValList)

        # print(valList)
        # print(valListTail)
        # print(commonValList)
        # print(rotatedValList)
        # print(f"hash {hash:#08x} hashTail {hashTail:#08x} rotatedHash {rotatedHash:#08x}")

        test_cases = [
            {"write": [valList, valListTail], "expected": hash, "tail" : False, "rotate" : 0},
            {"write": [valList, valListTail], "expected": hashTail, "tail" : True, "rotate" : 0},
            {"write": [valList, valListTail], "expected": rotatedHash, "tail" : False, "rotate" : rotateNum},
        ]

        print("\n--- Запуск тестов hash ---")
        results = []
        for case in test_cases:
            results.append(self.exec.hash_test(case))
        print("\nТесты hash завершены:", "OK" if all(results) else "FAIL")
        return all(results)

class QueueTests:
    def __init__(self, ops):
        self.exec = QueueExecutor(ops)

    def hash_tests(self):
        valList = generate_values(10)
        hash = calc_hash(valList)

        # print(valList)
        # print(f"hash {hash:#08x}")

        test_cases = [
            {"write": valList, "expected": hash},
        ]

        print("\n--- Запуск тестов hash ---")
        results = []
        for case in test_cases:
            results.append(self.exec.hash_test(case))
        print("\nТесты hash завершены:", "OK" if all(results) else "FAIL")
        return all(results)

class RbtreeTests:
    def __init__(self, ops):
        self.exec = RbtreeExecutor(ops)

    def hash_tests(self):
        valList = generate_values(10)
        valList = list(set(valList))
        valList.sort()
        valListLast = valList.copy()
        valListLast.reverse()
        hash = calc_hash(valList)
        hashLast = calc_hash(valListLast)

        # print(valList)
        # print(valListLast)
        # print(f"hash {hash:#08x} hashLast {hashLast:#08x}")

        test_cases = [
            {"write": valList, "expected": hash, "last" : False},
            {"write": valListLast, "expected": hashLast, "last" : True},
        ]

        print("\n--- Запуск тестов hash ---")
        results = []
        for case in test_cases:
            results.append(self.exec.hash_test(case))
        print("\nТесты hash завершены:", "OK" if all(results) else "FAIL")
        return all(results)
