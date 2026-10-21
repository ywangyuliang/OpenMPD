# Tasking and collectives

Simple examples of combinations that are currently unsupported.

| Program | Case | Result |
| --- | --- | --- |
| `data_before_task.c` | `cluster data broad(x)` before the tasks, within the same `cluster`. | Translation error. |
| `data_after_task.c` | `cluster data broad(x)` after the tasks, within the same `cluster`. | Translation error. |
| `broadcast_with_tasks.c` | `cluster broad(x)` with tasks inside its body. | Does not terminate. |
| `broadcast_before_tasks.c` | A separate `cluster broad(x)` before the tasking region. | Does not terminate. |
| `broadcast_after_tasks.c` | A separate `cluster broad(x)` after the tasking region. | Does not terminate. |
| `barrier_after_tasks.c` | `MPI_Barrier(MPI_COMM_WORLD)` after the tasks. | Does not terminate. |
