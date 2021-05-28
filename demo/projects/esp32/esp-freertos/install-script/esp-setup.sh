#!/bin/bash

echo "**** Dev Environment Setup Started ****"
printf "Update Ubuntu packages\n"
sudo apt-get update
printf "Install Prerequisites\n"
sudo apt-get install gcc git wget make libncurses-dev flex bison gperf python python-pip python-setuptools python-serial python-cryptography python-future python-pyparsing
printf "\n"

#**Save Original Path - will need this when switching between versions**
printf "Create base path variable\n"
export ORIG_PATH=$PATH
printf "ORIG_PATH Environment variable set to original (pre-install) Path:\n"
printf "$ORIG_PATH \n"

#**Determine WSL version**
[ $(grep -oE 'gcc version ([0-9]+)' /proc/version | awk '{print $3}') -gt 5 ] &&  export wsl_ver="WSL2" || export wsl_ver="WSL1"
printf "Running WSL version: $wsl_ver \n"

#**Set up Azure Code Path**
if [ "$wsl_ver" == "WSL1" ]
then
    mkdir -p /mnt/c/azure-iot/
    export AzCodePath=/mnt/c/azure-iot
else 
    mkdir -p ~/azure-iot/
    export AzCodePath=~/azure-iot/
fi

printf "\nCode path for setup: $AzCodePath\n"
cd $AzCodePath

# **Create ESP directory**
printf "Create directory for ESP IDF SDK and Xtensa toolchain\n"
mkdir -p esp
cd esp
export ESP_INSTALL_PATH=$AzCodePath/esp

#**Set up ESP32 Toolchain**
cd ESP_INSTALL_PATH
printf "Getting toolchains for Legacy (3.3) and Stable (4.2)\n\n"
#wget https://dl.espressif.com/dl/xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz
printf "Starting toolchain download 3.3\n\n"
wget https://dl.espressif.com/dl/xtensa-esp32-elf-linux64-1.22.0-97-gc752ad5-5.2.0.tar.gz
printf "Starting toolchain download 4.2\n\n"
wget https://dl.espressif.com/dl/xtensa-esp32-elf-gcc8_4_0-esp-2020r3-linux-amd64.tar.gz
printf "Starting OpenOCD download\n\n"
wget https://github.com/espressif/openocd-esp32/releases/download/v0.10.0-esp32-20200709/openocd-esp32-linux64-0.10.0-esp32-20200709.tar.gz


#**Get ESP32 Latest SDK/APIs**
cd $ESP_INSTALL_PATH
mkdir -p legacy
cd legacy
printf "\nExtract 64 bit ESP32 Toolchain (legacy-3.3) to Folder\n\n"
tar -xzf "$ESP_INSTALL_PATH/xtensa-esp32-elf-linux64-1.22.0-97-gc752ad5-5.2.0.tar.gz"
printf "64 bit ESP32 Toolchain (legacy-3.3) Extraction Complete\n\n"
printf "Get Legacy version of ESP32 IDF/SDK v3.3\n"
git clone -b v3.3 --recursive https://github.com/espressif/esp-idf.git
printf "Legacy version of ESP32 SDK v3.3 Completed\n\n"

cd $ESP_INSTALL_PATH
mkdir -p stable
cd stable
printf "Extract 64 bit ESP32 Toolchain (stable-4.2) to Folder\n\n"
tar -xzf "$ESP_INSTALL_PATH/xtensa-esp32-elf-gcc8_4_0-esp-2020r3-linux-amd64.tar.gz"
printf "64 bit ESP32 Toolchain (stable-4.2) Extraction Complete\n\n"
printf "Get Stable version of ESP32 IDF/SDK v4.2\n"
git clone -b v4.2 --recursive https://github.com/espressif/esp-idf.git
printf "Stable version of ESP32 SDK v4.2 Completed\n\n"


#**Set ESP32 path for stable toolchain and ESP tools by default"
export STABLE_PATH=$ESP_INSTALL_PATH/stable

# ** Install OpenOCD tool ** #
cd $STABLE_PATH
mkdir -p tools 
cd tools 
printf "Extract OpenOCD for ESP32\n\n"
tar -xzf "$ESP_INSTALL_PATH/openocd-esp32-linux64-0.10.0-esp32-20200709.tar.gz"
printf "OpenOCD for ESP32 extraction complete\n\n"

# ## Create IDF Env Variables and Tool paths ##

#* Keep Original User Path **#
export ORIG_PATH=$PATH
printf "Original Path: $ORIG_PATH\n\n"

export IDF_PATH=$STABLE_PATH/esp-idf
printf "IDF_PATH: $IDF_PATH\n\n"

export IDF_TOOLS_PATH=$STABLE_PATH/tools
export ESP_IDF_TOOL_PATH=$IDF_PATH/tools
printf "IDF tools path: $IDF_TOOLS_PATH & ESP IDF tools path: $ESP_IDF_TOOL_PATH\n\n"


# ** Create PATHs for switching between Toolchain versions ** #
export IDF_STABLE_PATH=$STABLE_PATH/esp-idf
printf "4.2 toolchain stable ver-IDF_STABLE_PATH: $IDF_STABLE_PATH\n"
export IDF_LEGACY_PATH=$ESP_INSTALL_PATH/legacy/esp-idf
printf "3.3 toolchain legacy ver-IDF_LEGACY_PATH: $IDF_LEGACY_PATH\n"

printf "Creating links to tools\n\n"
cd $IDF_TOOLS_PATH
ln -sf $IDF_PATH/components/esptool_py/esptool esptool
printf "esptool link successfully created\n\n"

ln -sf $IDF_PATH/components/espcoredump ./espcoredump
printf "ESP Core dump directory link created in tools directory\n\n"

ln -sf $IDF_PATH/components/partition_table ./partition_table
printf "ESP Partition Table directory link created in tools directory\n\n"

ln -sf $IDF_PATH/components/app_update ./app_update
printf "App Update tool directory link created in tools directory\n\n"

export IDF_OOCD_PATH=$IDF_TOOLS_PATH/openocd-esp32/bin
printf "ESP OpenOCD path: $IDF_OOCD_PATH\n\n"

#**Setup PATH for ESP toolchain, SDK, Tools
export XTENSA_PATH=$STABLE_PATH/xtensa-esp32-elf/bin
printf "XTENSA PATH: $XTENSA_PATH\n\n"

printf "Updating PATH w/ESP toolchain and IDF\n\n"
export PATH=$XTENSA_PATH:$IDF_PATH:$ESP_IDF_TOOL_PATH:$IDF_TOOLS_PATH:$IDF_OOCD_PATH:$ORIG_PATH
printf "The updated Path is: $PATH\n\n"

printf "Update python requirements for ESP32 -- Python 2\n\n"
# /usr/bin/python -m pip install --user -r $IDF_PATH/requirements.txt
# printf "Update to python requirements completed for Python 2\n\n"

# ##printf "Update python requirements for ESP32 - Python 3\n\n"
# ##/usr/bin/python3 -m pip install --user -r $IDF_PATH/requirements.txt
printf "Updated path with ESP IDF and Toolchain:\n $PATH\n\n"
echo "Install/Update python requirements for ESP Stable IDF & SDK\n"
/usr/bin/python -m pip install --user -r $IDF_PATH/requirements.txt
printf "**** Dev Environment Setup Completed ****\n"


