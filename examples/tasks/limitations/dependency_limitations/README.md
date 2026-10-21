# Dependency limitations

Simple examples of current limitations in task dependencies.

| Program | Case | Result |
| --- | --- | --- |
| `array_section.c` | Array section with more than one element. | Generated C fails to compile. |
| `alias.c` | Two names refer to the same memory location. | Prints `y = 0` instead of `10`. |
| `reader_writer.c` | A later writer can run before an earlier reader. | May print `y = 20` instead of `10`. |
