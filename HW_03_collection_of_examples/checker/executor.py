from module_ops import ModuleOps

class ListExecutor:
    def __init__(self, ops):
        self.ops = ops

    def hash_test(self, case):
        list = case['write'][0]
        listTail = case['write'][1]
        self.ops.write_cmd("clear")
        for value in reversed(list) :
            self.ops.write_cmd(f"add {value}")
        for value in listTail :
            self.ops.write_cmd(f"addtail {value}")
        for i in range(case['rotate']) :
            self.ops.write_cmd("rotate")
        self.ops.write_cmd("hashtail" if case['tail'] else "hash")
        value = self.ops.read_value()
        self.ops.write_cmd("size")
        size = self.ops.read_value()
        expectedSize = len(list) + len(listTail)
        print(f"list_hash_test: записано head {len(list)}, tail {len(listTail)}, прочитано hash {value:#08x} size {size}, ожидалось hash {case['expected']:#08x} size {expectedSize}")
        return value == case['expected']

class QueueExecutor:
    def __init__(self, ops):
        self.ops = ops

    def hash_test(self, case):
        list = case['write']
        self.ops.write_cmd("reset")
        for value in list :
            self.ops.write_cmd(f"add {value}")
        self.ops.write_cmd("hash")
        value = self.ops.read_value()
        self.ops.write_cmd("len")
        size = self.ops.read_value()
        expectedSize = len(list)
        print(f"queue_hash_test: записано head {len(list)}, прочитано hash {value:#08x} length {size}, ожидалось hash {case['expected']:#08x} length {expectedSize}")
        return value == case['expected']

class RbtreeExecutor:
    def __init__(self, ops):
        self.ops = ops

    def hash_test(self, case):
        list = case['write']
        self.ops.write_cmd("clear")
        for value in list :
            self.ops.write_cmd(f"add {value}")
        self.ops.write_cmd("hashlast" if case['last'] else "hash")
        value = self.ops.read_value()
        self.ops.write_cmd("size")
        size = self.ops.read_value()
        expectedSize = len(list)
        print(f"rbtree_hash_test: записано head {len(list)}, прочитано hash {value:#08x} length {size}, ожидалось hash {case['expected']:#08x} length {expectedSize}")
        return value == case['expected']
