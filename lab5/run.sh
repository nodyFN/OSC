#!/bin/bash

TARGET=${1:-board}
KERNEL_BIN="kernel.bin"
SEND_SCRIPT="send_kernel.py"
QEMU_LOG="qemu_output.log"
DEFAULT_BOARD_DEV="/dev/ttyUSB0"

echo ">> Cleaning..."
make clean > /dev/null

if [ "$TARGET" == "qemu" ]; then
    echo ">> Building for QEMU..."
    rm -f $QEMU_LOG
    
    make qemu > $QEMU_LOG 2>&1 &
    QEMU_PID=$!
    
    echo ">> QEMU started (PID: $QEMU_PID). Waiting for PTY..."

    PTY_DEV=""
    while [ -z "$PTY_DEV" ]; do
        if ! kill -0 $QEMU_PID 2>/dev/null; then
            echo "Error: QEMU exited unexpectedly. Check $QEMU_LOG"
            exit 1
        fi
        
        PTY_DEV=$(grep -o "/dev/pts/[0-9][0-9]*" $QEMU_LOG | head -n 1)
        sleep 0.5
    done

    echo ">> QEMU PTY detected at: $PTY_DEV"

    python3 $SEND_SCRIPT $PTY_DEV

    echo ">> Killing QEMU..."
    kill $QEMU_PID

elif [ "$TARGET" == "board" ]; then
    echo ">> Building for Board..."
    make board
    
    echo ""
    echo ">> Build complete."
    
    DEV=$DEFAULT_BOARD_DEV
    if [ ! -e "$DEV" ]; then
        read -p ">> Device $DEV not found. Enter UART device path: " DEV
    fi

    sudo python3 $SEND_SCRIPT $DEV

else
    echo "Usage: ./run.sh [qemu|board]"
    exit 1
fi

rm -f $QEMU_LOG