package cui

import "errors"

// ErrSkeletonPresentationUnavailable indicates that the selected library lacks
// the complete and compatible responsive Skeleton V1 group.
var ErrSkeletonPresentationUnavailable = errors.New("responsive Skeleton V1 is unavailable")

// SkeletonWidthMode selects how width resolves inside the parent layout.
type SkeletonWidthMode uint32

const (
	SkeletonFixed SkeletonWidthMode = iota
	SkeletonFraction
	SkeletonGrow
)

// SkeletonConfig is an owned, copyable configuration. Dimensions use logical
// pixels except fractional width (0, 1]. Grow requires width zero. MaxWidth is
// optional and applies to fixed and grow. Duration is in seconds; zero uses two.
// Set CustomColor to use numeric 0xRRGGBBAA, including transparent black.
type SkeletonConfig struct {
	WidthMode                       SkeletonWidthMode
	Width, Height, MaxWidth, Radius float32
	Duration                        float32
	BackgroundRGBA                  uint32
	CustomColor, Secondary          bool
	ReducedMotion                   bool
}

// DefaultSkeletonConfig makes a full-width, 16-pixel-high placeholder.
func DefaultSkeletonConfig() SkeletonConfig {
	return SkeletonConfig{WidthMode: SkeletonFraction, Width: 1, Height: 16}
}

// SkeletonPresentation is a copy of one successfully rendered sample. Times
// are absolute on the Core clock. NextWakeSeconds is zero for static motion.
type SkeletonPresentation struct {
	FrameSequence             uint64
	CoreTime, NextWakeSeconds float64
	Phase, Opacity            float32
}

// ResponsiveSkeleton creates a decorative loading placeholder with explicit
// responsive presentation. Use Skeleton for the legacy visual-size path.
func ResponsiveSkeleton(key string, config SkeletonConfig) *Node {
	n := Skeleton(key)
	n.err = n.ConfigureSkeleton(config)
	return n
}

// ConfigureSkeleton replaces this placeholder's presentation. Mounted nodes
// must be used on the app's owner thread. A failed change preserves the config.
func (n *Node) ConfigureSkeleton(config SkeletonConfig) error {
	if n == nil || n.kind != 35 || n.removed {
		return errors.New("responsive presentation requires a live Skeleton")
	}
	if n.hasVisualSize {
		return errors.New("legacy visual size cannot be combined with responsive Skeleton presentation")
	}
	if err := config.validate(); err != nil {
		return err
	}
	if n.app != nil {
		if err := n.configureMountedSkeleton(config); err != nil {
			return err
		}
	}
	n.skeletonConfig = &config
	return nil
}

func (c SkeletonConfig) validate() error {
	if !(c.Height > 0 && c.Height <= 16384 &&
		c.MaxWidth >= 0 && c.MaxWidth <= 16384 &&
		c.Radius >= 0 && c.Radius <= 16384 && c.Duration >= 0 && c.Duration <= 60) ||
		(!c.CustomColor && c.BackgroundRGBA != 0) {
		return errors.New("invalid responsive Skeleton dimensions, duration or color")
	}
	switch c.WidthMode {
	case SkeletonFixed:
		if c.Width > 0 && c.Width <= 16384 {
			return nil
		}
	case SkeletonFraction:
		if c.Width > 0 && c.Width <= 1 && c.MaxWidth == 0 {
			return nil
		}
	case SkeletonGrow:
		if c.Width == 0 {
			return nil
		}
	}
	return errors.New("invalid responsive Skeleton width")
}
