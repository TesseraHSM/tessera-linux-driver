
# Tessera Integrated HSM Linux Driver

---

## Overview

This directory contains the Linux kernel driver source code for the **Tessera Integrated HSM** device. The driver builds as an out-of-tree kernel module named `tsra.ko`.

---

## Build Instructions

### Prerequisites

- Linux kernel headers (or source tree) for the target kernel
- GCC, Make, and build essentials

### Build

By default the module is built against the running kernel's headers at
`/lib/modules/$(uname -r)/build`:

```bash
make
```

This generates a `tsra.ko` file.

### Building Against a Specific Kernel

Override `KDIR` to build against a different kernel headers or source tree:

```bash
make KDIR=/usr/src/linux-headers-$(uname -r)
```

### Clean

```bash
make clean
```

### Install

```bash
sudo make modules_install
```

---

## License

This driver is licensed under the **GNU General Public License, version 2 or (at your option) any later version** (`GPL-2.0-or-later`). See [LICENSE](LICENSE) for the full license text.

