import csv
from collections import defaultdict


END = ".END"


def read_csv():
    with open("io-table.csv", "r") as file:
        reader = csv.reader(file)
        header = next(reader)
        rows   = tuple((row[2], int(row[0], base = 16), int(row[1], base = 16)) for row in reader)
        return rows


def make_trie(rows):
    root = {}

    for name, io_addr, mem_addr in rows:
        current_dict = root
        for letter in name:
            if letter not in current_dict:
                current_dict[letter] = {}
            current_dict = current_dict[letter]
        current_dict[END] = mem_addr

    return root


def HEX(i):
    return f"0x{i:02X}"


def print_assembly(trie, label = "IO_LOOKUP_TREE", depth = 0):
    # Base Case
    if type(trie) != dict:
        return

    keys = sorted([key for key in trie.keys()])
    end  = trie[END] if keys[0] == END else 0
    keys = keys[1:]  if keys[0] == END else keys

    print(f"{label}:")
    print(f"\t.db HIGH({HEX(end)}), LOW({HEX(end)})")

    for key in keys:
        l = f"{label}_{key}" if depth == 0 else f"{label}{key}"
        print(f"\t.db \"{key}\", 0, HIGH({l} << 1), LOW({l} << 1)")

    print(f"\t.db 0, 0")

    for key in keys:
        l = f"{label}_{key}" if depth == 0 else f"{label}{key}"
        print_assembly(trie[key], l, depth + 1)


rows = read_csv()
trie = make_trie(rows)

print_assembly(trie)
