#!/usr/bin/python3

with open("sram.bin", "wb") as outfile:
    with open("gbcam_sram.outs", "r") as infile:
        for line in infile.readlines():
            str_bytes = line.split()[1:]
            for b in str_bytes:
                print(b)
                outfile.write(bytes([int(b, 16)]))