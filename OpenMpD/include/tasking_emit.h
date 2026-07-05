#ifndef TASKING_EMIT_H
#define TASKING_EMIT_H

/*
 * Bridges completed tasking regions into the final generated output. It emits
 * generated task definitions through deferred slots and writes the executable
 * region body before releasing the tasking-region model.
 */

#include "cluster_stack.h"

/*
 * Emits a tasking region's generated global definitions into the deferred
 * output slot and its body into the generated output, then destroys the
 * region. No-op when the cluster has no tasking region
 */
void tasking_emit_finalize_region(cluster_stack *c);

/*
 * Emits a tasking region's body into the generated output and destroys the
 * region. No-op when the cluster has no tasking region
 */
void tasking_emit_region_body(cluster_stack *c);

#endif
