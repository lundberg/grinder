import sys
import serial.tools.list_ports

Import("env")


def detect_upload_port(filters: tuple[str, ...] = ("usb", "updi")) -> str:
    ports = [
        port
        for port in serial.tools.list_ports.comports()
        if any(filter in port.description.lower() for filter in filters)
    ]
    if not ports:
        print("Unable to detect serial port!")
        sys.exit(-1)

    port, *_ = ports
    print(f"Detected Upload Port: {port.device} ({port.description})")
    return port.device


def operation(op: str, *, filename: str) -> None:
    port = detect_upload_port()
    env["FILENAME"] = filename
    if (a := env["PROGRAM_ARGS"]):
        env["FILENAME"] = a[0]
    cmd = f"$UPLOADER -P {port} $UPLOADERFLAGS -U {op}"
    env.Execute(cmd)


def read_eeprom(*args: str, **kwargs: str) -> None:
    operation("eeprom:r:$FILENAME:i", filename="eeprom.hex")


def write_eeprom(*args: str, **kwargs: str) -> None:
    operation("eeprom:w:$FILENAME:i", filename="eeprom.hex")


def read_firmware(*args: str, **kwargs: str) -> None:
    operation("flash:r:$FILENAME:r", filename="firmware.bin")


def write_firmware(*args: str, **kwargs: str) -> None:
    operation("flash:w:$FILENAME:r", filename="firmware.bin")


env.AddCustomTarget(
    "backup-eeprom",
    None,
    read_eeprom,
    "Backup EEPROM to hex file, using -a <filename.hex>",
)
env.AddCustomTarget(
    "restore-eeprom",
    None,
    write_eeprom,
    "Restore EEPROM from hex file, using -a <filename.hex>",
)
env.AddCustomTarget(
    "backup-firmware",
    None,
    read_firmware,
    "Backup firmware to binary file, using -a <filename.bin>",
)
env.AddCustomTarget(
    "restore-firmware",
    None,
    write_firmware,
    "Restore firmware from binary file, using -a <filename.bin>",
)
