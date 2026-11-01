# Indexed pointer-region admission

`cui_event_hovered(ui, event_index, id)` is a read-only Core query for a pointer
event's position against a named semantic's **previous completed** clipped
region. Read the complete contract in `include/cui/cui.h`. This is useful for
release validation when later queued mouse or touch samples have already changed
the context's final pointer position. Existing `cui_hovered` behavior is unchanged.

Call during ordinary frame submission after opening the relevant named scopes.
A valid prior input snapshot is required. The query returns false outside that
phase, during measurement/prepaint/inert/layout-only submission, on a sticky
error or invalid prior snapshot, for invalid IDs/indexes, disabled/unknown target
semantics, nonfinite coordinates, and events other than POINTER_DOWN/MOVE/UP.
It does not treat SCROLL or accessibility actions as pointer samples. Clearing
an error alone cannot reconstruct a missing completed snapshot.

The event's original indexed coordinates determine admission; capture and the
context's final pointer position do not. Consumption and activation acceptance
are separate: a consumed event can still be geometrically admitted. Callers
retain their own consumed-event, drag ownership and value/callback policies.
This query does not consume, activate, focus, capture, cancel or replay input.

Prior clipping, prior modal eligibility, and currently active retained named
scope ancestors apply. The target's prior scope ancestry must also belong to
any active current modal, independently of the query caller's scope stack. A
newly opened disjoint modal therefore denies old background target regions.
The geometry remains prior input geometry; it is not current-frame paint bounds.

Higher interactive layers or paint roots occlude the region, including popup or
absolute descendants. Within the same layer and paint root, later unrelated
interactive siblings occlude; ordinary actual layout descendants such as a
Table's headers and grips do not. Ancestry uses Core's retained nearest layout
semantic parent, not a `details.parent` override. Focusable and pointer-activatable
semantics and explicit pointer-activation barriers participate. Disabled barriers
still occlude; ordinary disabled controls do not. Passive painted siblings and
custom nonsemantic paint are not pixel-occlusion evidence from this query.

The query allocates nothing, adds no context fields, and borrows no caller memory.
It performs bounded scans over the configured semantic/scope capacities and
retained layout ancestry. Its worst-case work grows with those capacities; use
it for selected event decisions rather than as a generic per-node observer.
There is no event propagation or dispatch architecture change. The API is C11;
foreign high-level builder facades are not introduced by this addition.

## Table touch behavior

An enabled movable column header sets the existing `touch_drag_priority` semantic
flag. A touch that starts there owns column reordering instead of ancestor pan.
Nonmovable headers and body content retain scroll arbitration. Disabled/loading
and disabled-column cases retain their existing mutation restrictions.

Table validates a TOUCH drop using the indexed query. Its mouse branch continues
to use `cui_hovered`; all midpoint, pinned-boundary, threshold and callback rules
remain unchanged. Short taps still sort where sorting is enabled. Multicontact
cancellation does not fabricate a move callback, and a later genuine touch can
start a new reorder. This behavior does not implement Dialog reopen cancellation
or other raw consumers' retained-state lifecycle policies.
