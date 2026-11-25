package cui

/*
#include <stdint.h>
*/
import "C"
import (
	"errors"
	"runtime/cgo"
)

type runState struct {
	app        *App
	afterFrame func(*App) error
	err        error
	panicValue any
	panicked   bool
}

//export goBridgeAfterFrame
func goBridgeAfterFrame(user C.uintptr_t) (status C.int) {
	state := cgo.Handle(user).Value().(*runState)
	completed := false
	defer func() {
		if !completed {
			state.panicValue = recover()
			state.panicked = true
			status = 3 // Stop C before resuming the panic on the Go side.
		}
	}()
	_, state.err = state.app.Dispatch()
	if errors.Is(state.err, ErrClosed) {
		state.err = nil
	}
	if state.err == nil && !state.app.closeRequested && state.afterFrame != nil {
		state.err = state.afterFrame(state.app)
	}
	completed = true
	if state.err != nil {
		return 3
	}
	return 0
}
