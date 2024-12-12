import os
import winreg
from tkinter import filedialog as tkFileDialog
from pathlib import Path
import zipfile
import ctypes
import sys

def set_env_var(name, value):
    try:
        key = winreg.OpenKey(winreg.HKEY_CURRENT_USER, 'Environment', 0, winreg.KEY_ALL_ACCESS)
    except WindowsError:
        key = winreg.CreateKey(winreg.HKEY_CURRENT_USER, 'Environment')
    winreg.SetValueEx(key, name, 0, winreg.REG_SZ, str(value))
    winreg.CloseKey(key)

def main():
    if not ctypes.windll.shell32.IsUserAnAdmin():
        print('Allow the launch as an administrator!')
        return ctypes.windll.shell32.ShellExecuteW(None, "runas", sys.executable, " ".join(sys.argv), None, 1)

 # Use the script's location to determine the repository root
    script_path = Path(__file__).resolve()
    git_repo_root = script_path.parent if script_path.parent.name == 'contrib' else script_path.parent.parent

    select = tkFileDialog.askopenfilename(
        title='Select the executable file from the GTA:SA installation directory',
        filetypes=[("GTA:SA", ".exe")], initialdir=os.environ.get('GTA_SA_DIR', '')
    )
    if not select:
        return input('You have not selected a file, we are leaving. Press Enter...')

    gta_sa_file = Path(select)
    gta_root_dir = gta_sa_file.parent.resolve()

    # Ensure plugins.zip exists
    plugins_zip_path = git_repo_root / 'plugins.zip'
    if not plugins_zip_path.exists():
        raise FileNotFoundError(f"File not found: {plugins_zip_path}")

    # Unpack zip into GTA:SA root directory
    print("Unpacking `plugins.zip` into GTA:SA root directory...")
    with zipfile.ZipFile(plugins_zip_path) as plugins_zip:
        plugins_zip.extractall(gta_root_dir)

    # Create symlinks
    config_name = input("Choose configuration to link (debug/release, default: debug): ") or 'debug'
    print(f"Creating the symlinks for `{config_name}` configuration...")
    config_bin_dir = git_repo_root / 'bin' / Path(config_name)
    print(f'{config_bin_dir=}')
    for filename in ('gta_reversed.pdb', 'gta_reversed.asi'):
        dst = gta_root_dir / 'scripts' / filename
        dst.unlink(missing_ok=True) # Delete symlink if it already exists
        # This fails [WinError 1314] if the script isn't run with admin rights [softlinks require it]
        os.symlink(config_bin_dir / filename, dst)

    print("The Env variables are changing... If VS is open with the Modern solution, restart VS")
    set_env_var('GTA_SA_EXE', gta_sa_file)
    set_env_var('GTA_SA_DIR', gta_root_dir)
    input('Done! Press Enter...')
    os.system('pause')

if __name__ == "__main__":
    main()
