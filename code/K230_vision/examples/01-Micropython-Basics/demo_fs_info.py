import os

# File mode constants (MicroPython-style)
S_IFMT  = 0o170000
S_IFDIR = 0o040000
S_IFREG = 0o100000

def sizeof_fmt(num, suffix='B'):
    for unit in ['', 'K', 'M', 'G', 'T']:
        if abs(num) < 1024.0:
            return "%3.1f%s%s" % (num, unit, suffix)
        num /= 1024.0
    return "%.1f%s%s" % (num, 'P', suffix)

root_files = os.listdir('/')
fs_info_list = []

for f in root_files:
    fs_path = '/' + f

    try:
        # Get file type
        st_mode = os.stat(fs_path)[0]
        f_type = st_mode & S_IFMT
        if f_type == S_IFDIR:
            f_type_str = "DIR"
        elif f_type == S_IFREG:
            f_type_str = "FILE"
        else:
            f_type_str = "OTHER"

        # Get filesystem stats
        fs_stat = os.statvfs(fs_path)
        total_bytes = fs_stat[0] * fs_stat[2]  # f_bsize * f_blocks
        info = "%s [%s] size=%s" % (
            fs_path, f_type_str,
            sizeof_fmt(total_bytes)
        )
        fs_info_list.append(info)
        print(info)
    except Exception as e:
        print(f"⚠️ Error accessing {fs_path}: {e}")
