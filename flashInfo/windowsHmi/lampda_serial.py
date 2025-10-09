import time
import serial
import serial.tools.list_ports
import requests
import shutil
import os
import re, subprocess

#read usb devices
import sys, platform

try:
    import win32file
except ImportError:
    win32file = None

# used to translate text strings
import TextLangage as tx

class Release:
    def __init__(self, api_response):
        r = api_response
        self.tag = r.get("tag_name")
        self.name = r.get("name")   # revision
        self.creation_date = r.get("created_at")
        self.updated_date = r.get("updated_at")
        self.published_date = r.get("published_at")
        self.asset_url = [asset["browser_download_url"] for asset in r.get("assets", [])]
        self.description = r.get("body")

class LampDa:
    def __init__(self):
        # signal a board without a program
        self.type = "unflashed"

        self.hardware_v = ""
        self.firmware_v = ""
        self.base_v = ""
        self.user_v = ""

def update_devices_linux(label):
    devices = []
    # detect all parameters
    device_re = re.compile(r"(\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s+(\S+)\s*(\S*)+\s*(\S*)\s*(\S*)", re.I)
    df = subprocess.check_output(['lsblk', '-f'])
    id_list = ["NAME", "FSTYPE", "FSVER", "LABEL", "UUID", "FSAVAIL", "FSUSE%", "MOUNTPOINTS"]
    for i in df.split(b'\n'):
        if i:
            info = device_re.match(i.decode('utf-8'))
            if info:
                dinfo = dict(zip(id_list, info.group(1,2,3,4,5,6,7,8)))
                # dinfo['device'] = '/dev/bus/usb/%s/%s' % (dinfo.pop('bus'), dinfo.pop('device'))
                devices.append(dinfo)

    # keep only the lamps
    final_devices = []
    for device in devices:
        if label in device['LABEL']:
            # check if we can mount it
            if device['MOUNTPOINTS'] == '':
                df = subprocess.check_output(["udisksctl", "mount", "-b", "/dev/"+device['NAME']])
                splitted = df.strip().split(b' ')
                device['MOUNTPOINTS'] = splitted[-1].decode('utf-8')

            if device['MOUNTPOINTS'] != '':
                final_devices.append(device['MOUNTPOINTS'])
                # we only care about one lamp anyway
                break;
    return final_devices

def update_device_windows(label):
    import psutil
    for part in psutil.disk_partitions(all=False):
        try:
            psutil.disk_usage(part.mountpoint)  # test si accessible
            # Récupérer le volume label via Windows
            import ctypes

            vol_name_buf = ctypes.create_unicode_buffer(1024)
            fs_name_buf = ctypes.create_unicode_buffer(1024)
            serial = ctypes.c_uint32()
            max_component_len = ctypes.c_uint32()
            file_sys_flags = ctypes.c_uint32()

            ctypes.windll.kernel32.GetVolumeInformationW(
                ctypes.c_wchar_p(part.device),
                vol_name_buf,
                ctypes.sizeof(vol_name_buf),
                ctypes.byref(serial),
                ctypes.byref(max_component_len),
                ctypes.byref(file_sys_flags),
                fs_name_buf,
                ctypes.sizeof(fs_name_buf),
            )
            print("Lampda module name on windows", vol_name_buf.value)
            if vol_name_buf.value == label:
                return part.mountpoint
        except Exception:
            continue

def find_drive_by_label(label="LMBDROOT"):
    try:
        import platform
        system = platform.system()
        if system == 'Windows':
            return update_device_windows(label)
        else:
            devices = update_devices_linux(label)
            if(len(devices) > 0):
                print("Lampda module name on Linux", devices[0])
                return devices[0]

    except Exception as e:
        print("Could not check drives: ", e)



def find_lampda(sublist=None):
    lampdas = []
    text = "Lamp-da"
    search_list = (
        [p.device for p in serial.tools.list_ports.comports()]
        if sublist is None
        else sublist
    )
    if sublist != []:
        for port in search_list:
            try:
                with serial.Serial(port, baudrate=115200, timeout=0.1, write_timeout=0.2) as ser:
                    ser.write(("h" + "\n").encode())
                    response = ser.readline().decode(errors="ignore").strip()
                    if text in response:
                        lampdas.append((port, get_lampda_version(ser)))

            except Exception as e:
                print("Port search Exception", e)

    drive = find_drive_by_label()
    if drive is not None:
        lampdas.append((drive, LampDa()))

    return lampdas


def get_lampda_version(ser):
    """search all version in the lampda"""
    info = {
        "hardware": None,
        "firmware": None,
        "base software": None,
        "user software": None,
        "type": None,
    }

    ser.write(("v\n").encode())

    data = True
    while data:
        try:
            data = ser.readline().decode(errors="ignore").strip()
            if data:
                for key in info.keys():
                    if key in data:
                        try:
                            info[key] = "v" + data.split(":")[-1]
                        except:
                            raise Exception(f"wrong formate data for {key} : {data}")
        except Exception as e:
            print(f"\n[Erreur lecture] {e}")
            break

    ser.write(("t\n").encode())

    data = True
    while data:
        try:
            data = ser.readline().decode(errors="ignore").strip()
            if data:
                if "unknown" in data:
                    info["type"] = "unknown"
                else:
                    info["type"] = data.split(" ")[-1]
            data = None

        except Exception as e:
            print(f"\n[Erreur lecture] {e}")
            break

    lampd = LampDa
    lampd.hardware_v = info["hardware"]
    lampd.firmware_v = info["firmware"]
    lampd.base_v = info["base software"]
    lampd.user_v = info["user software"]
    lampd.type = info["type"]

    return lampd


def get_releases(url="https://api.github.com/repos/BaptisteHudyma/Lamp-Da/releases"):
    response = requests.get("https://api.github.com/rate_limit")
    if response.status_code == 200:
        rate_limit_data = response.json()["rate"]

        # keep a bit of margin
        if rate_limit_data['remaining'] > 10:
            response = requests.get(url)
            response.raise_for_status()  # lève une erreur si problème réseau ou API
            releases = response.json()
            return [Release(r) for r in releases]

    return []


def get_most_recent_version(releases: list[Release]):
    """sort all releases version passed on argument and return the higher version number"""
    releases.sort(key=compare_version, reverse=True)
    return releases[0]


def compare_version(version: Release):
    """return a value that correspond to a version number
    xwrote as vx.y.z with max 7 digit"""
    base_puissance = 1000000
    ret = 0
    for i, c in enumerate(version.tag):
        val = 0
        try:
            val = int(c)
        except:
            continue
        ret += val * base_puissance
        base_puissance /= 2
    return ret

def resetLampda(ser):
    DFU_COMMAND = b"DFU\n"
    ser.write(DFU_COMMAND)

def flash_lampda(port, lampda:LampDa, asset=None, local=False, skip_reset=False):
    if not skip_reset:
        try:
            with serial.Serial(port, baudrate=115200, timeout=0.1, write_timeout=0.2) as ser:
                resetLampda(ser)
        except Exception as e:
            raise e
        time.sleep(5)

    disk = find_drive_by_label()
    if disk == None:
        raise Exception(tx.get_text_translation(tx.USB_DRIVE_NOT_FOUND_ID))
    else:
        if local:
            file_to_flash = asset[0]
        else:
            uf2 = [file for file in asset if lampda.type in file]
            if len(uf2) != 1:
                raise Exception("Too much files were found")
            else:
                file_to_flash = lampda.type + ".uf2"
                download_release(uf2[0], file_to_flash)
        try:
            print("flashing ", file_to_flash, " to ", disk)
            shutil.copy(file_to_flash, os.path.join(disk, os.path.basename(file_to_flash).split('/')[-1]))
        except Exception as e:
            print("Exeption while flashing !", e)
            disk = find_drive_by_label()
            if disk is not None:
                raise e


def download_release(http_path, file_name):
    """download the file {http_path] as {file_name}"""
    response = requests.get("https://api.github.com/rate_limit")
    if response.status_code == 200:
        rate_limit_data = response.json()["rate"]

        # keep a bit of margin
        if rate_limit_data['remaining'] > 10:
            response = requests.get(http_path)

            with open(file_name, "wb") as f:
                f.write(response.content)
                return
    raise Exception("Download limit exeeded")


if __name__ == "__main__":
    pass
