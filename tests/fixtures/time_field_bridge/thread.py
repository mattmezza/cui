"""Actual loaded-image owner checks, with owner thread alive until destruction."""
import ctypes as C
import sys
import threading


class Options(C.Structure):
    _fields_ = [('size', C.c_uint32), ('abi', C.c_uint32), ('backend', C.c_int),
                ('nodes', C.c_uint32), ('events', C.c_uint32),
                ('width', C.c_float), ('height', C.c_float),
                ('title', C.c_char_p), ('font', C.c_char_p)]


class Node(C.Structure):
    _fields_ = [('slot', C.c_uint32), ('generation', C.c_uint32)]


class Queue(C.Structure):
    _fields_ = [('size', C.c_uint32), ('version', C.c_uint32),
                ('capacity', C.c_uint32), ('reserved', C.c_uint32)]


lib = C.CDLL(sys.argv[1])
lib.cui_bridge_create.argtypes = [C.POINTER(Options), C.POINTER(C.c_int)]
lib.cui_bridge_create.restype = C.c_void_p
lib.cui_bridge_destroy.argtypes = [C.c_void_p]
lib.cui_bridge_step.argtypes = [C.c_void_p, C.c_float]
lib.cui_bridge_time_field_enable_v1.argtypes = [C.c_void_p, C.c_uint32]
lib.cui_bridge_time_field_get_config_v1.argtypes = [C.c_void_p, Node, C.c_void_p]
lib.cui_bridge_slider_queue_enable_v1.argtypes = [C.c_void_p, C.c_uint32, C.POINTER(Queue)]
assert lib.cui_bridge_time_field_versions() == 1
for slider_first in (False, True):
    options = Options(C.sizeof(Options), 1, 0, 0, 0, 640, 480, None, None)
    status = C.c_int(-1)
    app = lib.cui_bridge_create(C.byref(options), C.byref(status))
    assert app and status.value == 0
    queue = Queue(C.sizeof(Queue), 1, 4, 0)
    if slider_first:
        assert lib.cui_bridge_slider_queue_enable_v1(app, C.sizeof(queue), C.byref(queue)) == 0
    assert lib.cui_bridge_time_field_enable_v1(app, 1) == 0
    if not slider_first:
        assert lib.cui_bridge_slider_queue_enable_v1(app, C.sizeof(queue), C.byref(queue)) == 0
    errors = []

    def foreign():
        try:
            out = (C.c_uint32 * 154)(*[0xA5A5A5A5] * 154)
            out[0], out[1] = 616, 1
            before = bytes(out)
            assert lib.cui_bridge_time_field_get_config_v1(app, Node(), out) == 8
            assert bytes(out) == before
            assert lib.cui_bridge_time_field_enable_v1(app, 1) == 8
            assert lib.cui_bridge_slider_queue_enable_v1(app, C.sizeof(queue), C.byref(queue)) == 8
            assert lib.cui_bridge_step(app, .016) == 8
        except BaseException as error:
            errors.append(error)

    worker = threading.Thread(target=foreign)
    worker.start()
    worker.join()
    assert not errors, errors
    assert lib.cui_bridge_time_field_enable_v1(app, 1) == 0
    lib.cui_bridge_destroy(app)
print('TimeField/Slider actual owner checks in both enable orders PASS')
