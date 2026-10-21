# Task stack depth

Simple example of stack overflow with nested tasks and `taskwait`.

| Program | Case | Observed result |
| --- | --- | --- |
| `task_chain.c` | Depth argument `1000`. | Finishes normally. |
| `task_chain.c` | Depth `100000` (default). | Fails with a stack overflow, confirmed by AddressSanitizer. |
