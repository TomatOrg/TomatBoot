# TomatBoot configuration file

## Location of the config file

Limine scans for a config file on *the boot drive*. Every partition on the boot drive
is scanned sequentially (first partition first, last partition last) for the presence
of either a `/boot/tomatboot.cfg`, `/tomatboot.cfg`, `/boot/limine.cfg`, or a
`/limine.cfg` file, in that order.

Once the file is located, Limine will use it as its config file. Other possible
candidates in subsequent partitions or directories are ignored.

It is thus imperative that the intended config file is placed in a location that will
not be shadowed by another potentially candidate config file.

## Structure of the config file

The Limine configuration file is comprised of *assignments* and *entries*.

### Entries and sub-entries

*Entries* describe boot *entries* which the user can select in the *boot menu*.

An *entry* is simply a line starting with `:` followed by a newline-terminated
string.
Any *locally assignable* key that comes after it, and before another *entry*, or
the end of the file, will be tied to the *entry*.

### Assignments

*Assignments* are simple `KEY=VALUE` style assignments.
`VALUE` can have spaces and `=` symbols, without requiring quotations. New lines
are delimiters.

Some *assignments* are part of an entry (*local*), some other assignments are *global*.
*Global assignments* can appear anywhere in the file and are not part of an entry,
although usually one would put them at the beginning of the config.
Some *local assignments* are shared between entries using any *protocol*, while other
*local assignments* are specific to a given *protocol*.

Some keys take *URIs* as values; these are described in the next section.

*Globally assignable* keys are:
* `TIMEOUT` - Specifies the timeout in seconds before the first *entry* is automatically booted. If set to `no`, disable automatic boot. If set to `0`, boots default entry instantly (see `DEFAULT_ENTRY` key).
* `DEFAULT_ENTRY` - 1-based entry index of the entry which will be automatically selected at startup. If unspecified, it is `1`.

*Locally assignable (non protocol specific)* keys are:
* `PROTOCOL` - The boot protocol that will be used to boot the kernel. Valid protocols are: `linux`, `stivale`, `stivale2`, `chainload`.
* `KERNEL_CMDLINE` - The command line string to be passed to the kernel. Can be omitted.

*Locally assignable (protocol specific)* keys are:
* Linux protocol:
    * `KERNEL_PATH` - The URI path of the kernel.
    * `MODULE_PATH` - The URI path to a module (such as initramfs).

* stivale and stivale2 protocols:
    * `KERNEL_PATH` - The URI path of the kernel.
    * `MODULE_PATH` - The URI path to a module.
    * `MODULE_STRING` - A string to be passed to a module.
    * `KASLR` - If set to `yes`, it enables Kernel Address Layout Randomisation for 64-bit relocatable kernels.

* multiboot2 protocol:
    * `KERNEL_PATH` - The URI path of the kernel.
    * `MODULE_PATH` - The URI path to a module.
    * `MODULE_STRING` - A string to be passed to a module.
  
Note that one can define `MODULE_PATH` and `MODULE_STRING` variables multiple times to specify multiple modules. 
The entries will be matched in order. E.g.: the 1st module path entry will be matched to the 1st module string entry that 
appear, and so on.

## URIs

A URI is a path that Limine uses to locate resources in the whole system. It is
comprised of a *resource*, a *root*, and a *path*. It takes the form of:
```
resource://root/path
```

The format for `root` changes depending on the resource used.

A resource can be one of the following:
* `boot` - The `root` is the 1-based decimal value representing the partition on the boot drive (values of 5+ for MBR logical partitions). If omitted, the partition containing the configuration file on the boot drive is used. For example: `boot://2/...` will use partition 2 of the boot drive and `boot:///...` will use the partition containing the config file on the boot drive.
* `guid` - The `root` takes the form of a GUID/UUID, such as `guid://736b5698-5ae1-4dff-be2c-ef8f44a61c52/...`. The GUID is that of either a filesystem, when available, or a GPT partition GUID, when using GPT, in a unified namespace.
* `uuid` - Alias of `guid`.

A URI can optionally be prefixed by a `$` character to indicate that the file
pointed to be the URI is a gzip-compressed payload to be uncompressed on the
fly. E.g.: `$boot:///somemodule.gz`.
