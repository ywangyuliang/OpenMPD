# Dependency limitations

Simple examples of current limitations in task dependencies.

| Program | Case | Result |
| --- | --- | --- |
| `array_section.c` | Array section with more than one element. | Generated C fails to compile. |
| `alias.c` | Two names refer to the same memory location. | Prints `y = 0` instead of `10`. |
