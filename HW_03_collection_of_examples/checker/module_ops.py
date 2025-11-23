import sys

class ModuleOps:
    def __init__(self, cmd_file, value_file):
        self.cmd_file = cmd_file
        self.value_file = value_file

    def write_cmd(self, value:str):
        try:
            # print(f"cmd: {value}")
            self.cmd_file.seek(0)
            #
            # since buffered text I/O gets stuck in case exceptions, we had to switch
            # to unbuffered binary I/O
            #
            self.cmd_file.write(value.encode("utf-8"))
            self.cmd_file.flush()
        except Exception as e:
            print(f"write_cmd(): exception {e}")
            return str(e)

    def read_value(self):
        try:
            self.value_file.seek(0)
            return int(self.value_file.read().strip())
        except Exception as e:
            print(f"read_value(): exception {e}")
            return str(e)
