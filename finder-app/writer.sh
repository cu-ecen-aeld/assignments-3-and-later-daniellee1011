#!/bin/bash

# Check if the correct number of arguments is provided
if [ $# -ne 2 ]; then
    echo "Error: Two arguments are required: <writefile> <writestr>"
    exit 1
fi

# Assign arguments to variables for clarity
writefile=$1
writestr=$2

# Create the directory path if it doesn't exist
mkdir -p "$(dirname "$writefile")"

# Write the string to the file, overwriting any existing content
echo "$writestr" > "$writefile"

# Check if the file was created successfully
if [ $? -ne 0 ]; then
    echo "Error: Could not create or write to file $writefile"
    exit 1
fi

# Print success message
echo "Successfully wrote to $writefile"

# Exit with success
exit 0
