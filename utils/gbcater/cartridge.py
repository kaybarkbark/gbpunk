from __future__ import annotations

import csv
import typing

from pathlib import Path
from enum import Enum


class CGBFunctionality(Enum):
    CGBExtra = "Extra CGB Functions"
    CGBOnly = "CGB Only"
    CGBNone = "None"


class Mapper(Enum):
    Unknown = "UNKNOWN"
    ROM_ONLY = "ROM ONLY"
    ROM_RAM = "ROM+RAM"
    MBC1 = "MBC1"
    MBC2 = "MBC2"
    MBC3 = "MBC3"
    MBC4 = "MBC4"
    MBC5 = "MBC5"
    MBC6 = "MBC6"
    MBC7 = "MBC7"
    MMM01 = "MMM01"
    HuC1 = "HuC1"
    HuC3 = "HuC3"
    TAMA5 = "TAMA5"
    GOWIN = "GOWIN"
    GBCAMERA = "Game Boy Camera"


class Cart:
    """Hold all the metadata about a cartridge"""

    CART_TITLE_ADDR = 0x134
    CART_TITLE_LEN = 16
    MFG_CODE_ADDR = 0x13F
    MFG_CODE_LEN = 4
    CGB_FLAG_ADDR = 0x143
    LIC_CODE_ADDR = 0x144
    LIC_CODE_LEN = 2
    SGB_FLAG_ADDR = 0x146
    CART_TYPE_ADDR = 0x147
    ROM_BANK_SHIFT_ADDR = 0x148
    RAM_BANK_COUNT_ADDR = 0x149
    DEST_CODE_ADDR = 0x14A
    LGC_LIC_CODE_ADDR = 0x14B
    MASK_ROM_VER_ADDR = 0x14C

    ROM_BANKSIZE = 0x4000
    RAM_BANKSIZE = 0x1000

    def __init__(
        self,
        cart_type: str,
        mapper_type: Mapper,
        rom_banks: int,
        ram_banks: int,
        ram_size: int,
        cgb_func: CGBFunctionality,
        sgb_flag: bool,
        region: str,
        mask_rom_ver: int,
        title: str,
    ):
        self.cart_type: str = cart_type
        self.mapper_type: Mapper = mapper_type
        self.rom_banks: int = rom_banks
        self.ram_banks: int = ram_banks
        self.ram_size: int = ram_size
        self.rom_size: int = rom_banks * self.ROM_BANKSIZE
        self.title: str = title
        self.cgb_func: CGBFunctionality = cgb_func
        self.sgb_flag: bool = sgb_flag
        self.region: str = region
        self.mask_rom_ver: int = mask_rom_ver

    def __str__(self):
        return f"{self.title}: {self.cart_type}. ROM: {self.rom_size} Bytes ({self.rom_banks} Banks). RAM {self.ram_size} Bytes ({self.ram_banks} Banks)"

    @classmethod
    def from_bytes(cls, cart_data: bytes) -> Cart:
        """Generate cartridge from raw bytes from ROM"""
        cart_type, mapper = cls.convert_cart_mapper(cart_data[cls.CART_TYPE_ADDR])
        ramsize, rambanks = cls.calculate_ram_size(data=cart_data, mapper=mapper)
        if cart_data[cls.CGB_FLAG_ADDR] == 0x80:
            cgb_func = CGBFunctionality.CGBExtra
        elif cart_data[cls.CGB_FLAG_ADDR] == 0xC0:
            cgb_func = CGBFunctionality.CGBOnly
        else:
            cgb_func = CGBFunctionality.CGBNone
        return Cart(
            cart_type=cart_type,
            mapper_type=mapper,
            rom_banks=2 << cart_data[cls.ROM_BANK_SHIFT_ADDR],
            ram_size=ramsize,
            ram_banks=rambanks,
            title=cls.get_title(data=cart_data),
            cgb_func=cgb_func,
            sgb_flag=cart_data[cls.SGB_FLAG_ADDR] == 0x3,
            region="Japan"
            if not cart_data[cls.DEST_CODE_ADDR]
            else f"Non-Japan ({hex(cart_data[cls.DEST_CODE_ADDR])})",
            mask_rom_ver=cart_data[cls.MASK_ROM_VER_ADDR],
        )

    @classmethod
    def get_title(cls, data: bytes) -> str:
        """Pull the title out of the cart data, try to make it as printable as possible"""
        return cls.strip_nonprintable_bytes(data[0x134 : 0x134 + cls.CART_TITLE_LEN])

    @staticmethod
    def strip_nonprintable_bytes(data: bytes) -> str:
        """Strip everything that is not printable ascii"""
        ret = ""
        for datum in data:
            if ord(" ") <= datum <= ord("~"):
                ret += chr(datum)
        return ret

    @classmethod
    def from_rom_file(cls, file: Path) -> Cart:
        """Generate a cartridge from a ROM file"""
        return cls.from_bytes(file.read_bytes())

    @classmethod
    def calculate_ram_size(cls, data: int, mapper: Mapper) -> typing.Tuple[int, int]:
        """Calculate the RAM size of a cart. This is kind of random, so it needs to be a LUT"""
        ram_size = data[cls.RAM_BANK_COUNT_ADDR]
        # Handle MBC2 w/ battery backed RAM. Only 256 bytes, split among 512 4 bit memory locations
        if mapper == Mapper.MBC2:
            return 256, 1
        if ram_size == 2:
            return 2048, 1
        if ram_size == 3:
            return (cls.RAM_BANKSIZE) * 4, 4
        if ram_size == 4:
            return (cls.RAM_BANKSIZE) * 16, 16
        if ram_size == 5:
            return (cls.RAM_BANKSIZE) * 8, 8
        return 0, 0

    @classmethod
    def get_licensee(cls, cart_data: bytes) -> str:
        raise NotImplementedError

    @classmethod
    def get_mfg(cls, cart_data: bytes) -> str:
        raise NotImplementedError

    @classmethod
    def convert_cart_mapper(cls, data: int) -> typing.Tuple[str, Mapper]:
        """Take the cart type byte and translate it into a cart type and mapper"""
        if data == 0:
            return "ROM ONLY (0x0)", Mapper.ROM_ONLY
        if data == 1:
            return "MBC1", Mapper.MBC1
        if data == 2:
            return "MBC1+RAM", Mapper.MBC1
        if data == 3:
            return "MBC1+RAM+BATTERY", Mapper.MBC1
        if data == 5:
            return "MBC2", Mapper.MBC2
        if data == 6:
            return "MBC2+BATTERY", Mapper.MBC2
        if data == 8:
            return "ROM+RAM", Mapper.ROM_RAM
        if data == 9:
            return "ROM ONLY (0x9)", Mapper.ROM_ONLY
        if data == 11:
            return "MM01", Mapper.MMM01
        if data == 12:
            return "MMM01+RAM", Mapper.MMM01
        if data == 13:
            return "MMM01+RAM+BATTERY", Mapper.MMM01
        if data == 15:
            return "MBC3+TIMER+BATTERY", Mapper.MBC3
        if data == 16:
            return "MBC3+TIMER+RAM+BATTERY", Mapper.MBC3
        if data == 17:
            return "MBC3", Mapper.MBC3
        if data == 18:
            return "MBC3+RAM", Mapper.MBC3
        if data == 19:
            return "MBC3+RAM+BATTERY", Mapper.MBC3
        if data == 21:
            return "MBC4", Mapper.MBC4
        if data == 22:
            return "MBC4+RAM", Mapper.MBC4
        if data == 23:
            return "MBC4+RAM+BATTERY", Mapper.MBC4
        if data == 25:
            return "MBC5", Mapper.MBC5
        if data == 26:
            return "MBC5+RAM", Mapper.MBC5
        if data == 27:
            return "MBC5+RAM+BATTERY", Mapper.MBC5
        if data == 28:
            return "MBC5+RUMBLE", Mapper.MBC5
        if data == 29:
            return "MBC5+RUMBLE+RAM", Mapper.MBC5
        if data == 30:
            return "MBC5+RUMBLE+RAM+BATTERY", Mapper.MBC5
        if data == 0x20:
            return "MBC6", Mapper.MBC6
        if data == 0x22:
            return "MBC7+SENSOR+RUMBLE+RAM+BATTERY", Mapper.MBC7
        if data == 252:
            return "GB CAMERA", Mapper.GBCAMERA
        if data == 0xFD:
            return "TAMA5", Mapper.TAMA5
        if data == 0xFE:
            return "HuC3", Mapper.HuC3
        if data == 0xFF:
            return "HuC1+RAM+BATTERY", Mapper.HuC1
        return f"UNKNOWN ({hex(data)})", Mapper.Unknown


def folder_to_csv(folder: Path, output: Path):
    """Take a folder full of ROMs and catalogue them all. Useful for downloading packs from archive.org for data mining"""
    with open(output, "w") as csvfile:
        csvwriter = csv.writer(csvfile, delimiter=",")
        csvwriter.writerow(
            [
                "Filename",
                "Title",
                "Cart Type",
                "Mapper Type",
                "ROM Banks",
                "RAM Banks",
                "RAM Size",
                "ROM Size",
                "CGB Functionality",
                "SGB Functionality",
                "Mask ROM Ver",
                "Region",
            ]
        )
        for file in folder.glob("**/*"):
            if (
                file.suffix == ".gb" or file.suffix == ".bin" or file.suffix == ".gbc"
            ) and file.is_file:
                cart = Cart.from_rom_file(file=file)
                csvwriter.writerow(
                    [
                        file.name.strip(","),
                        cart.title.strip(","),
                        cart.cart_type,
                        cart.mapper_type.value,
                        str(cart.rom_banks),
                        str(cart.ram_banks),
                        hex(cart.ram_size),
                        hex(cart.rom_size),
                        cart.cgb_func.value,
                        "Yes" if cart.sgb_flag else "No",
                        cart.mask_rom_ver,
                        cart.region,
                    ]
                )
