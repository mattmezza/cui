// SPDX-License-Identifier: MIT
package main

/*
#include <stdlib.h>
#include "../host.h"
extern void changed(int count);
static void on_changed(void *user, int count) { (void)user; changed(count); }
static inline int run(const char *font, unsigned frames) {
    return cui_example_run(font, frames, on_changed, NULL);
}
*/
import "C"
import (
	"fmt"
	"os"
	"runtime"
	"strconv"
	"unsafe"
)

var observed int

//export changed
func changed(count C.int) { observed = int(count); fmt.Println("Go count:", observed) }
func main() {
	runtime.LockOSThread()
	var font *C.char
	var frames C.uint
	if len(os.Args) > 1 {
		font = C.CString(os.Args[1])
		defer C.free(unsafe.Pointer(font))
	}
	if len(os.Args) > 2 {
		n, err := strconv.ParseUint(os.Args[2], 10, 32)
		if err != nil {
			panic(err)
		}
		frames = C.uint(n)
	}
	result := C.run(font, frames)
	if result != 0 || (font == nil && observed != 2) {
		panic(fmt.Sprintf("CUI failure %d, count %d", result, observed))
	}
}
