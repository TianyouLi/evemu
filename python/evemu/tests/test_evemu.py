from ctypes.util import find_library
import os
import tempfile
import unittest

from evemu import (
    LIB, EvEmu, EvEmuBase, EvEmuDevice, EvEmuWrapper, WrapperError)


# This needs to get updated with new versions of evemu
LOCAL_LIB = "../src/.libs/libutouch-evemu.so.1.0.0"

# This should also be examined every release of evemu
API = [
    "evemu_new",
    "evemu_delete",
    "evemu_extract",
    "evemu_write",
    "evemu_read",
    "evemu_write_event",
    "evemu_record",
    "evemu_read_event",
    "evemu_play",
    "evemu_create",
    "evemu_destroy",
    # Device functions
    "evemu_get_version",
    "evemu_get_name",
    "evemu_get_id_bustype",
    "evemu_get_id_vendor",
    "evemu_get_id_product",
    "evemu_get_id_version",
    "evemu_get_abs_minimum",
    "evemu_get_abs_maximum",
    "evemu_get_abs_fuzz",
    "evemu_get_abs_flat",
    "evemu_get_abs_resolution",
    "evemu_has_prop",
    "evemu_has_event",
    ]


class BaseTestCase(unittest.TestCase):

    def setUp(self):
        library = find_library(LIB)
        if not library:
            library = LOCAL_LIB
        self.library = library
        self.device_name = "My Special Device"
        from evemu import tests
        basedir = tests.__path__[0]
        self.data_dir = os.path.join(basedir, "data", "ntrig-xt2")

    def get_device_file(self):
        return os.path.join(self.data_dir, "ntrig-xt2.device")

    def get_events_file(self):
        return os.path.join(self.data_dir, "ntrig-xt2-4-tap.event")

    def test_initialize(self):
        wrapper = EvEmuBase(self.library)
        # Make sure that the library loads
        self.assertNotEqual(
            wrapper._evemu._name.find("libutouch-evemu"), -1)
        # Make sure that the expected functions are present
        for function_name in API:
            function = getattr(wrapper._evemu, function_name)
            self.assertTrue(function is not None)


class EvEmuDeviceTestCase(BaseTestCase):

    def setUp(self):
        super(EvEmuDeviceTestCase, self).setUp()
        self.device = EvEmuDevice(self.library, self.device_name)

    def test_initialize(self):
        self.assertTrue(self.device._device, self.device_name)


class EvEmuWrapperTestCase(BaseTestCase):

    def setUp(self):
        super(EvEmuWrapperTestCase, self).setUp()
        self.wrapper = EvEmuWrapper(self.library)

    def test_initialize(self):
        self.assertTrue(self.wrapper._device_pointer is None)

    def test_new(self):
        self.wrapper.new(self.device_name)
        pointer = self.wrapper._device_pointer
        self.assertTrue(isinstance(pointer.contents.value, int))

    def test_read_no_device_pointer(self):
        filename = tempfile.mkstemp()
        self.assertRaises(WrapperError, self.wrapper.read, filename)

    def test_read(self):
        self.wrapper.new(self.device_name)
        # hrm... not sure if I should be reading from the device file or
        # preping an empty file...
        result = self.wrapper.read(self.get_device_file())
        # XXX need to do checks against the result

    # XXX file this test in
    def test_create_no_device_pointer(self):
        pass

    # XXX file this test in
    def test_create(self):
        pass

    def test_extract_no_device_pointer(self):
        filename = tempfile.mkstemp()
        self.assertRaises(WrapperError, self.wrapper.extract, filename)

    def test_extract(self):
        self.wrapper.new(self.device_name)
        result = self.wrapper.read(self.get_device_file())
        print result

class EvEmuTestCase(BaseTestCase):

    def setUp(self):
        super(EvEmuTestCase, self).setUp()
        self.evemu = EvEmu(library=self.library)

    def test_initialize(self):
        self.assertTrue(self.evemu._wrapper is not None)
        self.assertTrue(self.evemu._virtual_device is None)

    def test_describe(self):
        pass

    def test_create_device(self):
        # Load the device file
        pass

    def test_record(self):
        pass

    def test_play(self):
        #self.evemu.play(self.get_events_file(), self.get_device_file())
        pass


if __name__ == "__main__":
    unittest.main()
