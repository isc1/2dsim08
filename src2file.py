#!/usr/bin/env python3
import os
import argparse
import sys

# ==============================================================================
# Source Code Consolidator - Usage Instructions
# ==============================================================================
#
# Basic usage:
#   python source_consolidator.py /path/to/source/directory
#
# With line numbers:
#   python source_consolidator.py /path/to/source/directory -n
#
# All available options:
#   python source_consolidator.py /path/to/source/directory -o output_file.txt -n -e .c,.h,.cpp,.py
#
# Features:
#   1. Traverses directories recursively
#   2. Sorts files for consistent output
#   3. Handles encoding issues gracefully
#   4. Formats with requested header style (# === Filename === #)
#   5. Includes line numbers when requested with the `-n` flag
#   6. Customizable file extensions through the `-e` option
#   7. Custom output file name with the `-o` option
#   8. Auto-adjusts output filename when -n flag is used
# ==============================================================================

def parse_arguments():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description='Consolidate source code files into a single annotated file.'
    )
    parser.add_argument(
        'directory', 
        help='Directory containing source files to consolidate'
    )
    parser.add_argument(
        '-o', '--output', 
        default='consolidated_source.txt',
        help='Output file name (default: consolidated_source.txt, or consolidated_source_n.txt with -n)'
    )
    parser.add_argument(
        '-n', '--line-numbers', 
        action='store_true',
        help='Add line numbers to each line of source code'
    )
    parser.add_argument(
        '-e', '--extensions',
        default='.c,.h,.cpp,.hpp,.md',
        help='Comma-separated list of file extensions to include (default: .c,.h,.cpp,.hpp,.md)'
    )
    
    return parser.parse_args()

def get_output_filename(base_filename, add_line_numbers, user_specified_output):
    """
    Determine the output filename based on flags and user input.
    
    Args:
        base_filename (str): The default base filename
        add_line_numbers (bool): Whether line numbers are being added
        user_specified_output (str): User-specified output filename
    
    Returns:
        str: The final output filename
    """
    # If user explicitly specified an output file, use it as-is
    if user_specified_output != 'consolidated_source.txt':
        return user_specified_output
    
    # If using default filename and line numbers are requested, modify it
    if add_line_numbers:
        name, ext = os.path.splitext(base_filename)
        return f"{name}_n{ext}"
    
    return base_filename

def consolidate_source_files(directory, output_file, add_line_numbers=False, extensions=None):
    """
    Consolidate source files from the given directory into a single file.
    
    Args:
        directory (str): Directory path containing source files
        output_file (str): Output file path
        add_line_numbers (bool): Whether to add line numbers
        extensions (list): List of file extensions to include
    """
    if not os.path.isdir(directory):
        print(f"Error: '{directory}' is not a valid directory")
        sys.exit(1)
    
    # Normalize extensions to include the dot if missing
    normalized_extensions = []
    for ext in extensions:
        if not ext.startswith('.'):
            ext = '.' + ext
        normalized_extensions.append(ext.lower())
    
    # Collect source files
    source_files = []
    for root, _, files in os.walk(directory):
        for file in files:
            _, ext = os.path.splitext(file)
            if ext.lower() in normalized_extensions:
                source_files.append(os.path.join(root, file))
    
    # Sort files for consistent output
    source_files.sort()
    
    # Process files
    with open(output_file, 'w') as out_file:
        for source_file in source_files:
            rel_path = os.path.relpath(source_file, directory)
            
            # Write file header
            out_file.write(f"# === {rel_path} ===\n")
            
            # Write file content with optional line numbers
            try:
                with open(source_file, 'r', encoding='utf-8') as in_file:
                    if add_line_numbers:
                        for i, line in enumerate(in_file, start=1):
                            out_file.write(f"{i:4d} | {line}")
                    else:
                        out_file.write(in_file.read())
            except UnicodeDecodeError:
                out_file.write("# [Binary file or encoding not supported]\n")
            except Exception as e:
                out_file.write(f"# [Error reading file: {str(e)}]\n")
            
            # Write footer
            out_file.write("#\n\n")
    
    print(f"Consolidated {len(source_files)} files into '{output_file}'")

def main():
    args = parse_arguments()
    
    # Determine the actual output filename
    output_file = get_output_filename(args.output, args.line_numbers, args.output)
    
    # Split and clean extensions
    extensions = [ext.strip() for ext in args.extensions.split(',')]
    
    consolidate_source_files(
        args.directory,
        output_file,
        args.line_numbers,
        extensions
    )

if __name__ == "__main__":
    main()