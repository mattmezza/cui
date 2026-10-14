package main

import "testing"

func TestWorkspaceHeadless(t *testing.T) {
	if err := run(true, ""); err != nil {
		t.Fatal(err)
	}
}
