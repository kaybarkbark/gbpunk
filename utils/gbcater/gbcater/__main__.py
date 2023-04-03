from .utils import folder_to_csv
from pathlib import Path

# print(Cart.from_rom_file(Path("/home/k/Downloads/pokemon_yellow.gb")))
folder_to_csv(Path("/home/k/all_games"), Path("/home/k/all_games.csv"))
