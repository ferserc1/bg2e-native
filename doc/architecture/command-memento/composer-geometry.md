# Composer geometry

## Reversible transforms

`GeometryEdit::Operation` names axis conversion, centering, floor placement and
unit conversions. `TransformGeometry` computes fixed forward and inverse local
matrices on initial execution. For PutOnFloor the vertical translation is
derived from the initial geometry bounds once; redo uses that stored matrix,
not current bounds or selection. Axis conversion maps `(x,y,z)` to `(x,-z,y)`.
Positive uniform scale and proper rotation preserve winding, indices and both
UV sets. Normals/tangents rotate for axis conversion without normalization, so
their magnitude and zero-vector state survive. Floating-point inverse arithmetic
can accumulate error; bitwise identity is not promised.

CPU `Mesh` payload is deduplicated by identity, then every live document Drawable
using that mesh gets a prepared raster replacement. The batch builds/uploads all
candidates before publishing CPU swaps and raster exchanges. Failure before
publication leaves live state untouched. Unexpected partial exchange is
compensated; failed compensation faults history. Displaced buffers remain held
until a conservative drained boundary.

## Information-losing replacement

`ReplaceGeometry` stores old/new immutable CPU payloads for prepared UV2 or other
results that cannot be inverted. It preserves submesh count/identity; operations
changing that count reject. Captured capacity bounds prevent outside growth from
exceeding the admitted memory budget. Known producers stop before preparation.

Ray-tracing/acceleration realizations reject because opaque allocation size and
retirement have no adapter. SDK owners sharing mutable CPU geometry outside the
Composer document must synchronize/rebuild their own native representations.
Ordinary inverse operations keep weak identities and matrices instead of
permanent mesh copies; conservative temporary/recovery memory still counts
toward the entry reservation.
