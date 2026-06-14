import os
import re
import stat
import sys

# Пути к директориям
CURRENT_DIR = os.getcwd()
INCLUDE_DIR = os.path.join(CURRENT_DIR, "include")
SRC_DIR = os.path.join(CURRENT_DIR, "src")
OUTPUT_FILE = os.path.join(CURRENT_DIR, "..", "out", "ShardScript.hpp")

# Группа 1: захватывает '<' или '"'
# Группа 2: захватывает сам путь до файла
include_regex = re.compile(r'^\s*#include\s*([<"])([^>"]+)[>"]')
pragma_regex = re.compile(r"^\s*#pragma\s+once")


def get_all_files(directory, extensions):
    files_map = {}
    if not os.path.exists(directory):
        print(f"Warning: Directory {directory} does not exist.")
        return files_map

    for root, _, files in os.walk(directory):
        for file in files:
            if any(file.endswith(ext) for ext in extensions):
                full_path = os.path.join(root, file)
                rel_path = os.path.relpath(full_path, directory).replace("\\", "/")
                files_map[rel_path] = full_path
    return files_map


def parse_dependencies(headers):
    graph = {path: set() for path in headers}

    for rel_path, full_path in headers.items():
        try:
            with open(full_path, "r", encoding="utf-8") as f:
                for line in f:
                    match = include_regex.match(line)
                    if match:
                        dep = match.group(2)  # Берем именно путь (вторая группа)
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


def merge_files(sorted_headers, headers_map, sources_map, output_path):
    os.makedirs(os.path.dirname(output_path), exist_ok=True)

    system_includes = set()
    interface_content = []
    implementation_content = []

    # Белый список классических C-заголовков
    c_headers = {
        "assert.h",
        "complex.h",
        "ctype.h",
        "errno.h",
        "fenv.h",
        "float.h",
        "inttypes.h",
        "iso646.h",
        "limits.h",
        "locale.h",
        "math.h",
        "setjmp.h",
        "signal.h",
        "stdalign.h",
        "stdarg.h",
        "stdatomic.h",
        "stdbool.h",
        "stddef.h",
        "stdint.h",
        "stdio.h",
        "stdlib.h",
        "stdnoreturn.h",
        "string.h",
        "tgmath.h",
        "threads.h",
        "time.h",
        "uchar.h",
        "wchar.h",
        "wctype.h",
    }

    def is_stl_header(dep):
        """Определяет, является ли заголовок кроссплатформенным STL файлом."""
        # У C++ STL заголовков нет расширений и папок (например, vector, string, memory)
        if "." not in dep and "/" not in dep and "\\" not in dep:
            return True
        # C заголовки проверяем по белому списку
        if dep in c_headers:
            return True
        return False

    # --- 1. СЕКЦИЯ ИНТЕРФЕЙСА (HEADERS) ---
    for rel_path in sorted_headers:
        full_path = headers_map[rel_path]
        interface_content.append(f"// --- Begin Header: {rel_path} ---")

        with open(full_path, "r", encoding="utf-8") as f:
            for line in f:
                if pragma_regex.match(line):
                    continue

                match = include_regex.match(line)
                if match:
                    bracket_or_quote = match.group(1)
                    dep = match.group(2)

                    # Если это системный STL заголовок - забираем его наверх
                    if bracket_or_quote == "<" and is_stl_header(dep):
                        system_includes.add(dep)
                        interface_content.append(f"// #include <{dep}> (Moved to top)")
                        continue
                    # Если это локальный заголовок - просто комментируем
                    elif dep in headers_map:
                        interface_content.append(f'// #include "{dep}" (Merged)')
                        continue

                interface_content.append(line.rstrip())

        interface_content.append(f"// --- End Header: {rel_path} ---\n")

    # --- 2. СЕКЦИЯ РЕАЛИЗАЦИИ (SOURCES) ---
    for rel_path, full_path in sources_map.items():
        implementation_content.append(f"// --- Begin Source: {rel_path} ---")

        with open(full_path, "r", encoding="utf-8") as f:
            for line in f:
                if pragma_regex.match(line):
                    continue

                match = include_regex.match(line)
                if match:
                    bracket_or_quote = match.group(1)
                    dep = match.group(2)

                    if bracket_or_quote == "<" and is_stl_header(dep):
                        system_includes.add(dep)
                        implementation_content.append(
                            f"// #include <{dep}> (Moved to top)"
                        )
                        continue
                    elif dep in headers_map or dep in sources_map:
                        implementation_content.append(f'// #include "{dep}" (Merged)')
                        continue

                implementation_content.append(line.rstrip())

        implementation_content.append(f"// --- End Source: {rel_path} ---\n")

    # --- 3. ФОРМИРОВАНИЕ ИТОГОВОГО ФАЙЛА ---
    final_content = []
    final_content.append(
        "// ==========================================================================="
    )
    final_content.append("// ShardScript Single Header Library")
    final_content.append("// Auto-generated. Do not edit directly.")
    final_content.append("// ")
    final_content.append("// USAGE:")
    final_content.append(
        "// Include this file normally where you need the ShardScript API."
    )
    final_content.append(
        "// In EXACTLY ONE .cpp file, define SHARDSCRIPT_IMPLEMENTATION before including:"
    )
    final_content.append("// ")
    final_content.append("// #define SHARDSCRIPT_IMPLEMENTATION")
    final_content.append('// #include "ShardScript.hpp"')
    final_content.append(
        "// ===========================================================================\n"
    )

    final_content.append("#ifndef SHARDSCRIPT_SINGLE_HEADER_H")
    final_content.append("#define SHARDSCRIPT_SINGLE_HEADER_H\n")

    final_content.append("// Disable DLL linkage for Single Header build")
    final_content.append("#ifndef SHARDSCRIPT_STATIC")
    final_content.append("#define SHARDSCRIPT_STATIC")
    final_content.append("#endif\n")

    # Вставка собранных системных (STL) заголовков
    if system_includes:
        final_content.append("// --- Standard Library Includes ---")
        for inc in sorted(system_includes):
            final_content.append(f"#include <{inc}>")
        final_content.append("")  # Пустая строка для отступа

    final_content.extend(interface_content)
    final_content.append("#endif // SHARDSCRIPT_SINGLE_HEADER_H\n")

    final_content.append("#ifdef SHARDSCRIPT_IMPLEMENTATION\n")
    final_content.extend(implementation_content)
    final_content.append("#endif // SHARDSCRIPT_IMPLEMENTATION\n")

    if os.path.exists(output_path):
        os.chmod(output_path, stat.S_IWRITE)

    with open(output_path, "w", encoding="utf-8") as f:
        f.write("\n".join(final_content))

    print(
        f"Successfully merged {len(sorted_headers)} headers and {len(sources_map)} source files into {output_path}"
    )


def main():
    print("Scanning include directory...")
    headers_map = get_all_files(INCLUDE_DIR, [".h", ".hpp"])
    print(f"Found {len(headers_map)} header files.")

    print("Scanning src directory...")
    sources_map = get_all_files(SRC_DIR, [".cpp", ".c", ".cc"])
    print(f"Found {len(sources_map)} source files.")

    print("Analyzing header dependencies...")
    graph = parse_dependencies(headers_map)

    print("Sorting headers topologically...")
    sorted_headers = topological_sort(graph)

    print("Merging content into single header...")
    merge_files(sorted_headers, headers_map, sources_map, OUTPUT_FILE)


if __name__ == "__main__":
    main()
