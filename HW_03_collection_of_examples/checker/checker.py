import os
import sys

from module_ops import ModuleOps
from tests import ListTests, QueueTests, RbtreeTests

ModuleMap = {"list" : "ex_list", "queue" : "ex_queue", "rbtree" : "ex_rb_tree"}

class Checker:
    def __init__(self, module_map):
        self.module_name = module_map[1]
        self.cmd_path = f"/sys/module/{self.module_name}/parameters/cmd"
        self.value_path = f"/sys/module/{self.module_name}/parameters/value"
        self.cmd_file = None
        self.value_file = None
        self.open_sysfs_files()

        self.ops = ModuleOps(self.cmd_file, self.value_file)

        if (module_map[0] == "list") :
            self.tests = ListTests(self.ops)
        elif (module_map[0] == "queue") :
            self.tests = QueueTests(self.ops)
        elif (module_map[0] == "rbtree") :
            self.tests = RbtreeTests(self.ops)
        else :
            print("Unsupported test!")
            sys.exit(3)

    def open_sysfs_files(self, mode='r+'):
        try:
            #
            # since buffered text I/O gets stuck in case of exceptions, we had to switch
            # to unbuffered binary I/O
            #
            self.cmd_file = open(self.cmd_path, 'wb', buffering=0)
            self.value_file = open(self.value_path, 'r')
        except Exception as e:
            print(f"Ошибка при открытии файлов sysfs: {e}")
            sys.exit(1)

    def check_hash(self):
        return self.tests.hash_tests()
