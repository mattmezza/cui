package cui

import (
	"errors"
	"strings"
	"unicode/utf8"
)

// ErrPaginationOptionsUnavailable indicates that the bridge lacks the complete
// Pagination V1 configuration with a compatible record layout.
var ErrPaginationOptionsUnavailable = errors.New("Pagination V1 options are unavailable")

// PaginationConfig holds copied options for a retained pager. AvailableWidth is
// the current inner width in logical pixels, or zero for a single row.
// VisiblePages is 5–32 or zero for the default page window. Counter replaces the
// numbered buttons. JumpMenu enables opening ellipses in numbered mode.
// Chevrons pairs arrows with Previous/Next captions. IconOnly hides the captions.
// Empty labels use the defaults. Nonempty labels are at most 127 UTF-8 bytes.
type PaginationConfig struct {
	Counter, IconOnly, Chevrons, JumpMenu bool
	VisiblePages                          uint32
	AvailableWidth                        float32
	Group, First, Previous, Next, Last    string
	EarlierPages, LaterPages              string
}

// ConfiguredPagination creates a pager with explicit layout and navigation
// options. Use Pagination for an ordinary pager on earlier bridge providers.
func ConfiguredPagination(key string, config PaginationConfig) *Node {
	n := Pagination(key)
	n.err = n.ConfigurePagination(config)
	return n
}

// ConfigurePagination updates presentation while preserving the page and
// selection callback. A failed update preserves the previous configuration.
func (n *Node) ConfigurePagination(config PaginationConfig) error {
	if err := n.checkPagination(); err != nil {
		return err
	}
	if err := config.validate(); err != nil {
		return err
	}
	if n.app != nil {
		if err := n.setPaginationConfig(&config); err != nil {
			return err
		}
	}
	n.paginationConfig = &config
	return nil
}

// ResetPagination clears the presentation override. DesignSize, total and page
// are independent. A mounted pager requires a compatible V1 bridge.
func (n *Node) ResetPagination() error {
	if err := n.checkPagination(); err != nil {
		return err
	}
	if n.app != nil {
		if err := n.setPaginationConfig(nil); err != nil {
			return err
		}
	}
	n.paginationConfig = nil
	return nil
}

func (n *Node) checkPagination() error {
	if n == nil || n.kind != 42 || n.removed {
		return errors.New("options require a live Pagination")
	}
	return nil
}

func (c PaginationConfig) validate() error {
	if (c.VisiblePages != 0 && (c.VisiblePages < 5 || c.VisiblePages > 32)) ||
		!(c.AvailableWidth >= 0 && c.AvailableWidth <= 32768) {
		return errors.New("invalid Pagination page budget or available width")
	}
	for _, label := range []string{c.Group, c.First, c.Previous, c.Next, c.Last, c.EarlierPages, c.LaterPages} {
		if len(label) > 127 || !utf8.ValidString(label) ||
			strings.ContainsFunc(label, func(r rune) bool {
				return r < 32 || r == 127 || r == 0x85 || r == 0x2028 || r == 0x2029
			}) {
			return errors.New("Pagination labels must be single-line UTF-8 of at most 127 bytes")
		}
	}
	return nil
}
