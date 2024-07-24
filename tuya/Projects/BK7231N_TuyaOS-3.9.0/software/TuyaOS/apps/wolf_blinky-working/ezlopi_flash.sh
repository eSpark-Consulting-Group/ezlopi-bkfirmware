#!/bin/sh

APP_VERSION=$1
BUILD_CMD=$2
COM_PORT=$3
APP_PATH=$(pwd)

# Change directory to SDK path
SDK_PATH=${SDK_PATH:-"/home/maaz/Documents/tuya/Projects/BK7231N_TuyaOS-3.9.0/software/TuyaOS"} # Default SDK path if not set
cd "$SDK_PATH" || { echo "Failed to change directory to SDK path: $SDK_PATH"; exit 1; }

# Determine the absolute path of the project directory
ABSOLUTE_APP_PATH=$(realpath "$APP_PATH") || { echo "Failed to get absolute path of the project directory"; exit 1; }

# Calculate the relative path of the project directory from the SDK path
RELATIVE_APP_PATH=$(realpath --relative-to="$SDK_PATH" "$ABSOLUTE_APP_PATH") || { echo "Failed to calculate relative path"; exit 1; }

# Set the APP_NAME based on the project directory name
APP_NAME=$(basename "$RELATIVE_APP_PATH")

echo APP_PATH=$RELATIVE_APP_PATH
echo APP_NAME=$APP_NAME
echo APP_VERSION=$APP_VERSION
echo BUILD_CMD=$BUILD_CMD
echo COM_PORT=$COM_PORT
echo SDK_PATH=$SDK_PATH

fatal() {
    echo -e "\033[0;31merror: $1\033[0m"
    exit 1
}

[ -z $APP_VERSION ] && fatal "no version!"

# Function to build the application
build_app() {
    sh build_app.sh "$RELATIVE_APP_PATH" $APP_NAME $APP_VERSION
    if [ $? -ne 0 ]; then
        fatal "Build failed!"
    fi
}

# Function to clean the application build
clean_app() {
    sh build_app.sh "$RELATIVE_APP_PATH" $APP_NAME $APP_VERSION clean
    exit 0
}

# Function to flash the binary
flash_app() {
    BINARY_PATH="$APP_PATH/output/$APP_VERSION/${APP_NAME}_QIO_${APP_VERSION}.bin"
    if [ -f "$BINARY_PATH" ]; then
        cd hid_flasher || { echo "Failed to change directory to hid_flasher"; exit 1; }
        sudo python3 uartprogram $BINARY_PATH -d $COM_PORT -w -u -s 0x0
        if [ $? -ne 0 ]; then
            fatal "Flashing failed!"
        else
            echo "Flashing successful!"
        fi
    else
        fatal "Binary file not found: $BINARY_PATH"
    fi
}

# Handle the clean command
if [ "$BUILD_CMD" = "clean" ]; then
    clean_app
fi

# Handle the build command
if [ "$BUILD_CMD" = "build" ]; then
    build_app
fi

# Handle the build_flash command
if [ "$BUILD_CMD" = "build_flash" ]; then
    build_app
    if [ -n "$COM_PORT" ]; then
        flash_app
    else
        fatal "No COM port provided for flashing!"
    fi
fi

# Handle the flash command
if [ "$BUILD_CMD" = "flash" ]; then
    if [ -n "$COM_PORT" ]; then
        flash_app
    else
        fatal "No COM port provided for flashing!"
    fi
fi
