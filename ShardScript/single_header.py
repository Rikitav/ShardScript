import os
import re
import sys

INCLUDE_DIR = os.path.join(os.getcwd(), "include")
OUTPUT_FILE = os.path.join(os.getcwd(), "dist", "ShardScript.hpp")
ROOT_NAMESPACE = "ShardScript"

include_regex = re.compile(r'^\s*#include\s*[<"]([^>"]+)[>"]')
pragma_regex = re.compile(r'^\s*#pragma\s+once')

def get_all_headers(directory):
    headers = {}
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(".h") or file.endswith(".hpp"):
                full_path = os.path.join(root, file)
                rel_path = os.path.relpath(full_path, directory).replace("\\", "/")
                headers[rel_path] = full_path
    return headers

def parse_dependencies(headers):
    graph = {path: set() for path in headers}
    
    for rel_path, full_path in headers.items():
        try:
            with open(full_path, 'r', encoding='utf-8') as f:
                lines = f.readlines()
                for line in lines:
                    match = include_regex.match(line)
                    if match:
                        dep = match.group(1)
                        if dep in headers:
                            graph[rel_path].add(dep)
                            
        except Exception as e:
            print(f"Error reading {rel_path}: {e}")
            
    return graph

def topological_sort(graph):
    visited = set()
    temp_mark = set()
    sorted_list = []

    def visit(node):
        if node in temp_mark:
            print(f"Warning: Circular dependency detected involving {node}")
            return
        if node not in visited:
            temp_mark.add(node)
            for neighbor in graph.get(node, []):
                visit(neighbor)
            temp_mark.remove(node)
            visited.add(node)
            sorted_list.append(node)

    for node in graph:
        visit(node)
        
    return sorted_list

def merge_files(sorted_files, headers_map, output_path):
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    
    processed_content = []
    system_includes = set()
    
    # Заголовок файла
    processed_content.append(f"// ShardScript Single Header Library")
    processed_content.append(f"// Auto-generated. Do not edit directly.")
    processed_content.append(f"#ifndef SHARDSCRIPT_SINGLE_HEADER_H")
    processed_content.append(f"#define SHARDSCRIPT_SINGLE_HEADER_H")
    processed_content.append("")

    for rel_path in sorted_files:
        full_path = headers_map[rel_path]
        processed_content.append(f"// --- Begin: {rel_path} ---")
        
        with open(full_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
            for line in lines:
                # Пропускаем #pragma once
                if pragma_regex.match(line):
                    continue
                
                # Обработка инклюдов
                match = include_regex.match(line)
                if match:
                    dep = match.group(1)
                    if dep in headers_map:
                        processed_content.append(f"// #include \"{dep}\" (Merged)")
                        continue
                    else:
                        pass
                
                processed_content.append(line.rstrip())
        
        processed_content.append(f"// --- End: {rel_path} ---\n")

    processed_content.append("#endif // SHARDSCRIPT_SINGLE_HEADER_H")

    with open(output_path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(processed_content))
    
    print(f"Successfully merged {len(sorted_files)} files into {output_path}")

def main():
    print("Scanning headers...")
    headers = get_all_headers(INCLUDE_DIR)
    print(f"Found {len(headers)} header files.")
    
    print("Analyzing dependencies...")
    graph = parse_dependencies(headers)
    
    print("Sorting files...")
    sorted_files = topological_sort(graph)
    
    print("Merging content...")
    merge_files(sorted_files, headers, OUTPUT_FILE)

if __name__ == "__main__":
    main()