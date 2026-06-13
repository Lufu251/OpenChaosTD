#!/bin/bash

# ==========================================
# USER VARIABLES - EDIT THESE!
# ==========================================

# 1. Path to your Aseprite executable. 
# Typical Steam path on Linux is used below.
ASEPRITE_PATH="/home/Lukas/Games/SteamLibrary/steamapps/common/Aseprite/aseprite"

# 2. The .aseprite or .ase file you want to export from.
INPUT_FILE="/home/lukas/Workspace/OpenChaosTD/datapacks/default/resources/raw/openchaosdefender.aseprite"


# 3. The folder where you want to save the textures.
OUTPUT_DIR="/home/lukas/Workspace/OpenChaosTD/datapacks/default/resources/textures"

# 4. The output filename format.
OUTPUT_FILENAME="{slice}.png"

# ==========================================
# SCRIPT LOGIC (Do not edit below this line)
# ==========================================

echo "Starting Aseprite slice export..."

# Check if Aseprite executable exists
if ! command -v "$ASEPRITE_PATH" &> /dev/null && [ ! -f "$ASEPRITE_PATH" ]; then
    echo "Error: Aseprite not found at: $ASEPRITE_PATH"
    echo "Please update the ASEPRITE_PATH variable inside this script."
    exit 1
fi

# Check if input file exists
if [ ! -f "$INPUT_FILE" ]; then
    echo "Error: Input file '$INPUT_FILE' not found."
    echo "Please update the INPUT_FILE variable inside this script."
    exit 1
fi

# Run the Aseprite command
"$ASEPRITE_PATH" -b "$INPUT_FILE" --split-slices --save-as ""$OUTPUT_DIR/$OUTPUT_FILENAME""

# Check if the command succeeded
if [ $? -eq 0 ]; then
    echo "Success! All slices from '$INPUT_FILE' have been exported as '$OUTPUT_FORMAT'."
else
    echo "An error occurred while exporting."
    exit 1
fi