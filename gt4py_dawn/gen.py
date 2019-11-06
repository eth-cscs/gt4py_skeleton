import os
import sys

import jinja2

assert len(sys.argv) == 3

env = jinja2.Environment(loader=jinja2.FileSystemLoader("."))
template = env.get_template(sys.argv[1])

params = dict(
    stencil_unique_name="copy_stencil",
    module_name="gt4py_dawn_computation",
    backend="x86",
    constants=dict(),
    arg_fields=[
        dict(name="f_out"),
        dict(name="f_in"),
    ],
    parameters=dict(),
    headerpath="computation.hpp",
)

with open(sys.argv[2], "w") as f:
    f.write(template.render(params))

