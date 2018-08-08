import os

import jinja2

template_name = "gtcomputation.cpp.in"

env = jinja2.Environment(loader=jinja2.FileSystemLoader('.'))
template = env.get_template(template_name)

params = dict(module_name='gtcomputation',
              stencil_name='diag_diff',
              float_t='double',
              ndims=3,
              halo_size=1,
              input_params=list(enumerate(['f_out', 'f_in'])))

with open(template_name[:-3], 'w') as f:
  f.write(template.render(params))
