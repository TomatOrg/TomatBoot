# TomatBoot configuration file

The TomatBoot configuration file is comprised of *assignments* and *entries*.

*Entries* describe boot *entries* which the user can select in the *boot menu*.

An *entry* is simply a line starting with `:` followed by a newline-terminated
string.
Any *locally assignable* key that comes after it, and before another *entry*, or
the end of the file, will be tied to the *entry*.

*Assignments* are simple `KEY=VALUE` style assignments.
`VALUE` can have spaces and `=` symbols, without requiring quotations. New lines
are delimiters.

Some *assignments* are part of an entry (*local*), some other assignments are *global*.
*Global assignments* can appear anywhere in the file and are not part of an entry,
although usually one would put them at the beginning of the config.

## Globally assignable keys
* `TIMEOUT` - Specifies the timeout in seconds before the first *entry* is automatically booted, this overrides the value in the setup menu.
* `DEFAULT_ENTRY` - 0-based entry index of the entry which will be automatically selected at startup. If unspecified, it is 0, this overrides the one in the setup menu.

## Locally assignable (non protocol specific) keys
* `PROTOCOL` - The boot protocol that will be used to boot the kernel. Valid protocols are: `linux`, `stivale`, `stivale2`.
* `CMDLINE` - The command line string to be passed to the kernel. Can be omitted.
* `KERNEL_CMDLINE` - Alias of `CMDLINE`.
* `KERNEL_PATH` - The URI path of the kernel.

## Locally assignable (protocol specific) keys
### Linux
* `MODULE_PATH` - The URI path to the initramfs.

### MB2 & Stivale
* `MODULE_PATH` - The URI path to a module.
* `MODULE_STRING` - A string to be passed to a module.

Note that one can define these 3 variable multiple times to specify multiple modules.
The entries will be matched in order. E.g.: the 1st partition entry will be matched
to the 1st path and the 1st string entry that appear, and so on.

## URIs 
A URI is a path that TomatBoot uses to locate resources in the whole system. It is comprised of a resource, a root, and a path. It takes the form of:
```
resource://root/path
```
The format for `root` changes depending on the resource used.

A resource can be one of the following:
* `boot` - The `root` takes the form of a partition number in the boot drivel for example `boot://1/...` would be the boot 
drive at partition number 1. Partitions are 1-based. Omitting the partition is possible; for example `boot:///...` . Omitting
the partition makes TomatBoot use the boot partition.
* `guid` - The `root` takes the form of a GUID/UUID, such as `guid://736b5698-5ae1-4dff-be2c-ef8f44a61c52/....` It is a partition GUID.
