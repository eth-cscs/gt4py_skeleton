import os
import sys

import jinja2

assert len(sys.argv) == 3

env = jinja2.Environment(loader=jinja2.FileSystemLoader("."))
template = env.get_template(sys.argv[1])

params = dict(
    stencil_unique_name="new_computation",
    module_name="gt4py_gt_computation",
    backend="x86",
    k_axis=dict(n_intervals=1, offset_limit=1),
    halo_sizes=[0, 0, 0],
    constants=dict(),
    arg_fields=[
        dict(name="f_out", dtype="float", layout_id=0),
        dict(name="f_in", dtype="double", layout_id=1),
    ],
    tmp_fields=dict(),
    parameters=dict(),
    stage_functors={
        "diag_diff_1_functor": dict(
            args=[
                dict(name="f_out", access_type="inout", extent=None),
                dict(name="f_in", access_type="in", extent=[-1, 0, -1, 0]),
            ],
            regions=[
                dict(
                    interval_start=(0, 1),
                    interval_end=(1, -1),
                    body="eval(f_out()) = eval(f_in(-1, -1, 0));",
                )
            ],
        )
    },
    multi_stages=[dict(exec="forward", steps=[["diag_diff_1_functor"]])],
    level_offset_limit=1,
    input_params=[(0, "f_out", "float"), (1, "f_in", "double")],
    headerpath="computation.hpp",
)

with open(sys.argv[2], "w") as f:
    f.write(template.render(params))

