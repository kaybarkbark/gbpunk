from cartridge import Cart, folder_to_csv
from pathlib import Path

# print(Cart.from_rom_file(Path("/home/k/Downloads/pokemon_yellow.gb")))
folder_to_csv(Path("/home/k/Downloads/GB/USA"), Path("/home/k/usa_games.csv"))
folder_to_csv(Path("/home/k/Downloads/GB/Japan"), Path("/home/k/japan_games.csv"))
folder_to_csv(Path("/home/k/Downloads/GB/Europe"), Path("/home/k/eu_games.csv"))
folder_to_csv(Path("/home/k/Downloads/GB/Rev"), Path("/home/k/revisions.csv"))
folder_to_csv(
    Path("/home/k/Downloads/GB/Proto-unl-demo-beta-sample"),
    Path("/home/k/proto-unlisc.csv"),
)
