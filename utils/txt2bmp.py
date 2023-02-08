#!/usr/bin/python3
import typing
import re
from pathlib import Path

def str2bin(text: str) -> bytearray:
    outbytes = bytearray()
    for line in text.splitlines():
        str_bytes = line.split()[1:]
        for b in str_bytes:
            outbytes += bytes([int(b, 16)])
    return outbytes

def seperate_photos(text: str) -> typing.List[str]:
    return re.split("Photo \d+", text)

def file_to_bmp(ins: Path) -> None:
    photo_count = 0
    with open(ins, "r") as infile:
        for photo in seperate_photos(infile.read()):
            outbytes = str2bin(photo)
            with open(f"outs{photo_count}.bmp", "wb") as outbmp:
                outbmp.write(outbytes)
            photo_count+=1

def main():
    file_to_bmp(Path("all_photos.txt"))

if __name__ == "__main__":
    main()