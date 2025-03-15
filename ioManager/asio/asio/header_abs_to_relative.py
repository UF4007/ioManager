import os
import re


def calculate_relative_path(from_file, to_file):
    from_dir = os.path.dirname(from_file)
    return os.path.relpath(to_file, start=from_dir)


def process_include(file_path, project_root):
    # print("process include file : " + file_path)
    with open(file_path, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    new_lines = []
    modified = False

    for line in lines:
        isInclude = re.match(r'#\s*include\s*["<](.*?)[">]', line)
        if isInclude:
            included_path = isInclude.group(1)
            # print("include match : " + included_path)
            parts = re.split(r"/|\\",included_path)

            if parts[0] == os.path.basename(project_root):
                # print(parts)
                included_path = os.path.sep.join(parts[1:])
                absolute_path = os.path.join(project_root, included_path)
                print("abs path found : " + absolute_path + ", in file : " + file_path)
                relative_path = calculate_relative_path(file_path, absolute_path)
                relative_path = re.sub(r"\\", "/", relative_path)
                print("relative : " + relative_path)
                line = f'#include "{relative_path}"\n'
                modified = True

        new_lines.append(line)

    if modified:
        with open(file_path, 'w', encoding='utf-8') as f:
            f.writelines(new_lines)


def process_project(project_root):
    for root, _, files in os.walk(project_root):
        for file in files:
            if file.endswith(('.h', '.hpp', '.c', '.cpp', '.cc', '.ipp')):
                process_include(os.path.join(root, file), project_root)


if __name__ == "__main__":
    current_directory = os.path.dirname(os.path.abspath(__file__))
    print("current_dir: " + current_directory)
    process_project(current_directory)
    input("Press Enter to exit...")
