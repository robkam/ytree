## Building from Source

To build and install ytree from source on a Debian/Ubuntu-based system, you will need the necessary development libraries and build tools.

### 1. Install Build Dependencies

```bash
sudo apt-get update && \
sudo apt-get install -y \
    build-essential \
    libncurses-dev \
    libreadline-dev \
    libarchive-dev \
    pandoc
```

### 2. Compile and Install

From the root of the source directory, run the following commands:

```bash
# Compile the executable and generate the man page
make

# Install the executable and man page to system directories
sudo make install
```

### 3. Uninstalling

To remove the ytree executable and man page from your system, run:

```bash
sudo make uninstall
```
