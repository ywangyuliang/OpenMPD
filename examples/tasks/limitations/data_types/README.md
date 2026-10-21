# Task data types and size limits

Results checked with `mpicc -O2` and two and four MPI processes.

| Program | Case | Observed result |
| --- | --- | --- |
| `struct_capture.c` | Pass a structure to a task. | Generated C fails to compile. |
| `union_capture.c` | Pass a union to a task. | Generated C fails to compile. |
| `typedef_dependency.c` | Use an alias of `int` in a dependency. | Translation error. |
| `pointer_capture.c` | Pass a pointer to a task. | Compiles with a pointer-to-integer warning, then fails with a segmentation fault. |
| `float_dependency.c` | Use a `float` input dependency. | Prints `0` instead of `7`; the generated input variable is uninitialized, so the result can vary. |
| `supported_values.c` | Use `int`, `long`, `double`, and a single array element. | Prints the expected value `20`. |
| `local_struct.c` | Use a structure locally, without transferring it. | Prints the expected value `7`. |
| `runtime_input_limit.c` | Create a task with 1025 input bytes. | Runtime error: oversized task input. |
| `runtime_dependency_limit.c` | Register a dependency with 17 bytes. | Runtime error: invalid dependency value size. |
