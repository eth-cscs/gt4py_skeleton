import os
import sys

import jinja2

assert len(sys.argv) == 3

env = jinja2.Environment(loader=jinja2.FileSystemLoader("."))
template = env.get_template(sys.argv[1])

# constants are not in param in the dawn repository, because they need to be in the SIR only
# In the SIR, we can add them to the globals, and set them by compiling with the `--config=file.json`
# option and provide a file which defines the globals.
params = dict(
    stencil_unique_name="copy_stencil",
    module_name="gt4py_dawn_computation",
    backend="x86",
    arg_fields=[
        dict(name="f_out"),
        dict(name="f_in"),
    ],
    parameters=[
        dict(name="global_param", dtype="double"),
    ],
    headerpath="computation.hpp",
    dawn_backend="cxxnaive",
)

with open(sys.argv[2], "w") as f:
    f.write(template.render(params))

