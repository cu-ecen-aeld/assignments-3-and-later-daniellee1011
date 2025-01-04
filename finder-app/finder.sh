#!/bin/sh

# Check if the correct number of arguments is provided
if [ $# -ne 2 ]; then
    echo "Error: Two arguments are required: <directory> <search string>"
    exit 1
fi

# Assign arguments to variables for clarity
filesdir=$1
searchstr=$2

# Check if the first argument is a valid directory
if [ ! -d "$filesdir" ]; then
    echo "Error: $filesdir is not a directory."
    exit 1
fi

# Count the number of files in the directory and its subdirectories
num_files=$(find "$filesdir" -type f | wc -l)

# Count the number of matching lines
num_matching_lines=$(grep -r "$searchstr" "$filesdir" 2>/dev/null | wc -l)

# Print the results
echo "The number of files are $num_files and the number of matching lines are $num_matching_lines"

# Exit with success
exit 0
