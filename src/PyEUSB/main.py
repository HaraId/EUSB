import eusb
import win32con
import win32file
import winioctlcon
import msvcrt
import time
import math

# virtual host controller interface guid
device_guid = '{752808f4-a9e9-42f4-8115-6440c98713b1}'


def ctl_code(device_type, function, method, access):
    return (device_type << 16) | (access << 14) | (function << 2) | method


IOCTL_HID_MOVE_MOUSE = ctl_code(winioctlcon.FILE_DEVICE_UNKNOWN, 0x880,
                                winioctlcon.METHOD_BUFFERED, winioctlcon.FILE_ANY_ACCESS)
simple = dict(int_list=[1, 2, 3],
              text='string',
              number=3.44,
              boolean=True,
              none=None)
simple.

def main():
    device_path = eusb.get_device_by_guid(device_guid)
    print('Device Path: ', device_path)
    handle = win32file.CreateFile(device_path, win32file.GENERIC_READ | win32file.GENERIC_WRITE,
                                  win32file.FILE_SHARE_WRITE | win32file.FILE_SHARE_READ,
                                  None, win32con.OPEN_EXISTING, win32con.FILE_ATTRIBUTE_NORMAL, None)
    if handle == win32file.INVALID_HANDLE_VALUE:
        print('[ ERROR ] ')
        return

    angle = math.pi / 2.0
    radius = 10.0
    odx = 0.0 * radius
    ody = 1.0 * radius
    circle_speed = math.pi / 10.0

    while True:
        if msvcrt.kbhit():
            c = msvcrt.getch()
            if c == b'c':
                break
        time.sleep(0.05)
        angle += circle_speed

        dx = math.cos(angle) * radius
        dy = math.sin(angle) * radius

        off_x = int(dx - odx)
        off_y = int(dy - ody)

        msg = b'\x00' + off_x.to_bytes(1, byteorder='big', signed = True) \
              + off_y.to_bytes(1, byteorder='big', signed = True) + b'\x00'
        result = win32file.DeviceIoControl(handle, IOCTL_HID_MOVE_MOUSE, msg, None, None)

        odx = dx
        ody = dy

    # message = b'Hello from user space'
    # (res, written) = win32file.WriteFile(handle, message)

if __name__ == '__main__':
    eusb.init_eusb()
    main()
    eusb.free_eusb()
