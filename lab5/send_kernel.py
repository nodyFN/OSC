import serial
import sys
import os
import struct
import time
import select
import tty
import termios

DEFAULT_SERIAL_PORT = '/dev/ttyUSB0'
BAUD_RATE = 115200
KERNEL_PATH = 'kernel.bin'
START_SIGNAL = b'OSC_START_DOWNLOAD'
DONE_SIGNAL  = b'OSC_DOWNLOAD_DONE'

def perform_upload(ser):
    print("\n\n>> Signal received! Sending kernel...\n")
    time.sleep(0.5)

    if not os.path.exists(KERNEL_PATH):
        print(f"\r\nError: {KERNEL_PATH} not found.\r\n")
        return

    with open(KERNEL_PATH, 'rb') as f:
        kernel_data = f.read()
    
    size = len(kernel_data)
    
    ser.write(struct.pack('<I', size))
    time.sleep(0.1)

    CHUNK_SIZE = 1024
    total_sent = 0
    for i in range(0, size, CHUNK_SIZE):
        chunk = kernel_data[i:i+CHUNK_SIZE]
        ser.write(chunk)
        total_sent += len(chunk)
        
        msg = f"\rProgress: {total_sent}/{size} bytes"
        sys.stdout.write(msg)
        sys.stdout.flush()
        
        time.sleep(0.005)

    sys.stdout.write("\r\nKernel sent successfully! Waiting for confirmation...\r\n")
    sys.stdout.flush()

def start_terminal(port):
    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=0.1)
        print(f"Opened {port} at {BAUD_RATE}")
    except Exception as e:
        print(f"Failed to open serial port: {e}")
        sys.exit(1)

    print("\n" + "="*50)
    print(">> Serial Terminal Active (Raw Mode)")
    print(">> Type 'load' to trigger kernel upload.")
    print(">> Press 'Ctrl + ]' to exit.")
    print("="*50 + "\n")

    if port != '/dev/ttyUSB0': 
        print("Connecting to Bootloader...")
        ser.write(b'\r')
        time.sleep(0.1)
    else:
        print("---------- Press Reset Button On Board ----------")


    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)

    signal_buffer = b''

    try:
        tty.setraw(sys.stdin.fileno())
        
        while True:
            r, _, _ = select.select([ser, sys.stdin], [], [])

            if ser in r:
                try:
                    data = ser.read(ser.in_waiting or 1)
                    if not data: break

                    signal_buffer += data
                    

                    if len(signal_buffer) > 200:
                        signal_buffer = signal_buffer[-100:]

                    if START_SIGNAL in signal_buffer:
                        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
                        perform_upload(ser)
                        tty.setraw(sys.stdin.fileno())

                        signal_buffer = b''
                        continue

                    if DONE_SIGNAL in signal_buffer:
                        signal_buffer = b''

                    sys.stdout.buffer.write(data)
                    sys.stdout.buffer.flush()

                except OSError:
                    break

            if sys.stdin in r:
                char = sys.stdin.read(1)
                # (Ctrl + ])
                if char == '\x1d': 
                    break
                ser.write(char.encode())

    except Exception as e:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        print(f"\r\nTerminal Error: {e}")
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        ser.close()
        print("\n>> Exited Terminal.")

if __name__ == '__main__':
    target_port = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_SERIAL_PORT
    start_terminal(target_port)