def ascii_to_hex(i):
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

for i in range(0, 256, 2):
    a = str(ascii_to_hex(i)).rjust(3)
    b = str(ascii_to_hex(i + 1)).rjust(3)
    print(f".db {a}, {b}")
