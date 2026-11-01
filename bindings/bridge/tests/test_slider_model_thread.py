"""Actual shared-library owner rejection; no UI, dependencies or thread-ID guesses."""
import ctypes as C
import sys
import threading


class Options(C.Structure):
    _fields_ = [("size", C.c_uint32), ("abi", C.c_uint32), ("backend", C.c_int),
                ("nodes", C.c_uint32), ("events", C.c_uint32),
                ("width", C.c_float), ("height", C.c_float),
                ("title", C.c_char_p), ("font", C.c_char_p)]


class Node(C.Structure):
    _fields_ = [("slot", C.c_uint32), ("generation", C.c_uint32)]


class Queue(C.Structure):
    _fields_ = [("size", C.c_uint32), ("schema", C.c_uint32),
                ("capacity", C.c_uint32), ("reserved", C.c_uint32)]


lib = C.CDLL(sys.argv[1])
lib.cui_bridge_create.argtypes = [C.POINTER(Options), C.POINTER(C.c_int)]
lib.cui_bridge_create.restype = C.c_void_p
lib.cui_bridge_destroy.argtypes = [C.c_void_p]
lib.cui_bridge_slider_queue_enable_v1.argtypes = [C.c_void_p, C.c_uint32, C.POINTER(Queue)]
lib.cui_bridge_slider_model_get_value_v1.argtypes = [C.c_void_p, Node, C.c_uint32, C.c_void_p]
options = Options(C.sizeof(Options), lib.cui_bridge_abi_version(), 0, 0, 0, 640, 480, None, None)
status = C.c_int(-1)
app = lib.cui_bridge_create(C.byref(options), C.byref(status))
assert app and status.value == 0
queue = Queue(C.sizeof(Queue), 1, 16, 0)
assert lib.cui_bridge_slider_queue_enable_v1(app, C.sizeof(queue), C.byref(queue)) == 0
results = []


def foreign_owner():
    output = (C.c_ubyte * 24)(*[0xA5] * 24)
    # Exact stale-node validation is later than owner validation. No borrowed UI is touched.
    results.append(lib.cui_bridge_slider_model_get_value_v1(app, Node(), 24, output))
    results.append(bytes(output))
    results.append(lib.cui_bridge_slider_queue_enable_v1(app, C.sizeof(queue), C.byref(queue)))


worker = threading.Thread(target=foreign_owner)
worker.start()
worker.join()
assert results == [8, bytes([0xA5] * 24), 8], results
assert lib.cui_bridge_slider_queue_enable_v1(app, C.sizeof(queue), C.byref(queue)) == 0
lib.cui_bridge_destroy(app)
print("actual bridge wrong-thread rejection/output preservation PASS")
