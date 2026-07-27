import os

print("Root directory:")
for item in os.listdir('/'):
    print(f"- {item}")
    try:
        for subitem in os.listdir('/' + item):
            print(f"  - {subitem}")
    except OSError:
        pass  # Skip if not a directory
    except UnicodeError:
        pass
