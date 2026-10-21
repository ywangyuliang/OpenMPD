# Code before runtime initialization

| Program | Case | Result |
| --- | --- | --- |
| `before_runtime_init.c` | A print before the first tasking region and another after `taskwait`. | The first message appears once per process; the second appears only once. |
