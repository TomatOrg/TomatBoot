import os
import sys
import pytest

sys.path.insert(0, os.path.abspath('liblab'))
from liblab import VM, System, Disk, SerialPort


@pytest.fixture
def vm():
    return VM([
        System(ram_mib=4096),
        Disk('../bin/test.img'),
        SerialPort(ident='serial')
    ])


@pytest.fixture
def serial(vm):
    return vm['serial'].pty


def test_stivale(serial):
    pass
