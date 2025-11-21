#!/usr/bin/env python3
import os
import sys
import re
from clang.cindex import Index, CursorKind, TranslationUnit

# Ensure libclang library can be found
# If you encounter issues finding libclang, uncomment below and set the correct path
import clang.cindex
clang.cindex.Config.set_library_path('C:/Program Files/LLVM/bin')

def should_add_inline(cursor):
    """Determine if inline keyword should be added to this cursor"""
    # Check if inline keyword already exists
    has_inline = False
    has_static = False
    is_coroutine = False
    
    tokens = list(cursor.get_tokens())
    for token in tokens:
        if token.spelling == 'inline':
            has_inline = True
        if token.spelling == 'static':
            has_static = True
        # Check if it contains co_await, co_yield or co_return keywords, indicating it's a coroutine function
        if token.spelling in ['co_await', 'co_yield', 'co_return']:
            is_coroutine = True
    
    # If it already has inline keyword, no need to add it again
    if has_inline:
        return False
    
    # Function declarations (including class member functions)
    if cursor.kind in [CursorKind.FUNCTION_DECL, CursorKind.CXX_METHOD, 
                      CursorKind.CONSTRUCTOR, CursorKind.DESTRUCTOR]:
        # If it's a coroutine function, add inline
        if is_coroutine:
            return True
            
        # Only process functions with a body
        children = list(cursor.get_children())
        has_body = any(child.kind == CursorKind.COMPOUND_STMT for child in children)
        
        # For static member functions, add inline even if they don't have a body
        if has_static and cursor.kind == CursorKind.CXX_METHOD:
            return True
            
        # For functions with a body, add inline
        if has_body:
            return True
    
    # Global variables or static variables
    elif cursor.kind == CursorKind.VAR_DECL:
        # Check if the variable is inside a function
        # If the parent is a function or method, it's a local variable
        parent = cursor.semantic_parent
        if parent and parent.kind in [CursorKind.FUNCTION_DECL, CursorKind.CXX_METHOD, 
                                     CursorKind.CONSTRUCTOR, CursorKind.DESTRUCTOR]:
            # Don't add inline to function-local static variables
            return False
            
        # For static variables outside functions, add inline
        if has_static:
            return True
            
        # For variables outside of classes and functions, add inline
        if cursor.semantic_parent.kind in [CursorKind.NAMESPACE, CursorKind.TRANSLATION_UNIT]:
            return True
    
    return False

def add_inline_to_file(file_path):
    """Add inline keyword to functions and variables in the file"""
    print(f"Processing file: {file_path}")
    
    # Parse file with options to ignore includes
    index = Index.create()
    try:
        # Add options to speed up parsing
        parse_options = (TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD | 
                         TranslationUnit.PARSE_INCOMPLETE)
        
        # Add arguments to help with parsing
        args = ['-fno-delayed-template-parsing', '-x', 'c++', '-std=c++20']
        
        # Skip includes by adding a dummy include path
        args.append('-I/dummy/include/path')
        
        tu = index.parse(file_path, args=args, options=parse_options)
    except Exception as e:
        print(f"Error parsing file {file_path}: {e}")
        return
    
    if not tu:
        print(f"Unable to parse file: {file_path}")
        return
    
    # Collect positions where inline needs to be added
    inline_positions = []
    
    def traverse(cursor):
        # Only process code in the current file
        if cursor.location.file and cursor.location.file.name == file_path:
            # print(f"Examining: {cursor.kind.name} - {cursor.spelling} at line {cursor.location.line}")
            
            # Check if inline needs to be added
            if should_add_inline(cursor):
                # Get the position of function or variable declaration
                start = cursor.extent.start
                inline_positions.append((start.line, start.column))
                # print(f"  -> Will add inline to: {cursor.spelling} at line {start.line}")
        
        # Recursively process child nodes
        for child in cursor.get_children():
            traverse(child)
    
    traverse(tu.cursor)
    
    if not inline_positions:
        print(f"No functions or variables need inline keyword in file {file_path}")
        return
    
    # Read file content
    with open(file_path, 'r', encoding='utf-8') as f:
        lines = f.readlines()
    
    # Sort by line number in descending order, so we can modify from back to front without affecting line numbers
    inline_positions.sort(reverse=True)
    
    # Add inline keyword
    for line_num, col_num in inline_positions:
        if line_num <= 0 or line_num > len(lines):
            continue
        
        line = lines[line_num - 1]
        # Insert inline keyword at appropriate position
        # Find the start of declaration (skip leading whitespace and storage class specifiers like static)
        match = re.search(r'^(\s*)(static\s+)?', line)
        if match:
            prefix = match.group(0)
            rest = line[len(prefix):]
            lines[line_num - 1] = f"{prefix}inline {rest}"
    
    # Write back to file
    with open(file_path, 'w', encoding='utf-8') as f:
        f.writelines(lines)
    
    print(f"File {file_path} processed, added {len(inline_positions)} inline keywords")

def process_directory(directory):
    """Process all C++ files in the directory"""
    extensions = ['.h', '.hpp', '.c', '.cpp', '.cc', '.ipp']
    
    for root, _, files in os.walk(directory):
        for file in files:
            if any(file.endswith(ext) for ext in extensions):
                file_path = os.path.join(root, file)
                add_inline_to_file(file_path)

if __name__ == "__main__":
    current_directory = os.path.dirname(os.path.abspath(__file__))
    print(f"Processing directory: {current_directory}")
    
    process_directory(current_directory)
    
    print("Processing complete!")
    input("Press Enter to exit...") 