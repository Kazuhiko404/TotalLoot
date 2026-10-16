import os
import shutil

mod_header = bytes.fromhex('110000001e0000000100000000000000000000000000000000000000010000000000000000000900000000000000')

re_kenshi_json = '{\n  "Plugins" : [ "TotalLoot.dll" ]\n}\n'

dll_src = r'f:\Proyectos\Kenshi-mods\TotalLoot\x64\Release\TotalLoot.dll'
ini_src = r'f:\Proyectos\Kenshi-mods\TotalLoot\TotalLoot.ini'

# 1. ModPackage dir
pkg_dir = r'f:\Proyectos\Kenshi-mods\TotalLoot\ModPackage\TotalLoot'
os.makedirs(pkg_dir, exist_ok=True)

with open(os.path.join(pkg_dir, 'TotalLoot.mod'), 'wb') as f:
    f.write(mod_header)

with open(os.path.join(pkg_dir, 'RE_Kenshi.json'), 'w', encoding='utf-8') as f:
    f.write(re_kenshi_json)

shutil.copy2(dll_src, os.path.join(pkg_dir, 'TotalLoot.dll'))
# The package always ships the current default config.
shutil.copy2(ini_src, os.path.join(pkg_dir, 'TotalLoot.ini'))
print('Packaged in:', pkg_dir)

# 2. Game mods dir
game_mod_dir = r'D:\SteamLibrary\steamapps\common\Kenshi\mods\TotalLoot'
os.makedirs(game_mod_dir, exist_ok=True)

with open(os.path.join(game_mod_dir, 'TotalLoot.mod'), 'wb') as f:
    f.write(mod_header)

with open(os.path.join(game_mod_dir, 'RE_Kenshi.json'), 'w', encoding='utf-8') as f:
    f.write(re_kenshi_json)

shutil.copy2(dll_src, os.path.join(game_mod_dir, 'TotalLoot.dll'))
# Never overwrite a config the player may have edited.
game_ini = os.path.join(game_mod_dir, 'TotalLoot.ini')
if not os.path.exists(game_ini):
    shutil.copy2(ini_src, game_ini)
    print('Installed default config:', game_ini)
else:
    print('Kept existing config:', game_ini)
print('Installed in:', game_mod_dir)
