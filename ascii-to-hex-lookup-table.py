def ascii_to_int(i):
    ASCII_ZERO  = 0x30
    ASCII_NINE  = 0x39
    ASCII_A_UPR = 0x41
    ASCII_F_UPR = 0x46
    ASCII_A_LWR = 0x61
    ASCII_F_LWR = 0x66
    INVALID     = 0xFF

    if i >= ASCII_ZERO and i <= ASCII_NINE:
        return i - ASCII_ZERO

    if i >= ASCII_A_UPR and i <= ASCII_F_UPR:
        return i - ASCII_A_UPR + 10

    if i >= ASCII_A_LWR and i <= ASCII_F_LWR:
        return i - ASCII_A_LWR + 10

    return INVALID

def ascii_to_hex(i):
    return f"0x{ascii_to_int(i):02X}"

for i in range(0, 256, 2):
    a = ascii_to_hex(i)
    b = ascii_to_hex(i + 1)
    print(f".db {a}, {b}")
