from __future__ import annotations


import csv

from pathlib import Path


UNKNOWN_STR: str = "UNKNOWN"


def str2bool(ins: str) -> bool:
    """Turn a str representation of a bool into a bool"""
    return True if "true" in ins.casefold() else False


def folder_to_csv(folder: Path, output: Path):
    """
    Take a folder full of ROMs and catalogue them all. Useful for downloading packs from archive.org for data mining
    """
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
                "Battery",
                "Timer",
                "Rumble",
                "Sensor",
                "Licensee",
                "Old Licensee Code",
                "CGB Functionality",
                "SGB Functionality",
                "Mask ROM Ver",
                "Region",
                "Weird",
            ]
        )
        print(f"Discovered {len(list(folder.glob('**/*')))} files")
        for file in folder.glob("**/*"):
            if (
                file.suffix == ".gb" or file.suffix == ".bin" or file.suffix == ".gbc"
            ) and file.is_file:
                cart = Cart.from_rom_file(file=file)
                print(f"Cataloguing {cart}...")
                csvwriter.writerow(
                    [
                        file.name.strip(","),
                        cart.title.strip(","),
                        str(cart.hardware),
                        cart.hardware.mapper.value,
                        str(cart.rom_banks),
                        str(cart.ram_banks),
                        hex(cart.ram_size),
                        hex(cart.rom_size),
                        str(cart.hardware.battery),
                        str(cart.hardware.timer),
                        str(cart.hardware.rumble),
                        str(cart.hardware.sensor),
                        cart.licensee,
                        cart.old_licensee_flag,
                        cart.cgb_func.value,
                        str(cart.sgb_flag),
                        cart.mask_rom_ver,
                        cart.region,
                        str(cart.is_weird)
                    ]
                )
