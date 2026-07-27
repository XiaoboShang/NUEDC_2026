import os
import time

def format_size(num, suffix='B'):
    for unit in ['', 'K', 'M', 'G', 'T']:
        if abs(num) < 1024.0:
            return "%3.1f%s%s" % (num, unit, suffix)
        num /= 1024.0
    return "%.1f%s%s" % (num, 'P', suffix)

def format_time(ts):
    try:
        t = time.localtime(ts)
        return "%04d-%02d-%02d %02d:%02d:%02d" % (t[0], t[1], t[2], t[3], t[4], t[5])
    except:
        return "???"

def tree(path='/', level=0, max_level=None, prefix=''):
    try:
        items = os.listdir(path)
    except Exception as e:
        print(f"{prefix}❌ [Error opening {path}]: {e}")
        return

    items.sort()
    for i, name in enumerate(items):
        full_path = path + '/' + name if not path.endswith('/') else path + name
        connector = '└── ' if i == len(items) - 1 else '├── '

        try:
            stat = os.stat(full_path)
            mode = stat[0]
            size = stat[6]
            mtime = stat[8]
            is_dir = mode & 0o170000 == 0o040000
        except Exception as e:
            print(f"{prefix}{connector}{name} ❌ stat failed: {e}")
            continue

        suffix = '/'
        if not is_dir:
            suffix = ''
        print(f"{prefix}{connector}{name}{suffix}  [{format_size(size)}]  {format_time(mtime)}")

        if is_dir and (max_level is None or level + 1 < max_level):
            new_prefix = prefix + ('    ' if i == len(items) - 1 else '│   ')
            tree(full_path, level + 1, max_level, new_prefix)

print("📂 Tree for /sdcard (max depth 3):")
tree('/sdcard', max_level=3)

print("\n📂 Tree for /data (unlimited depth):")
tree('/data')
