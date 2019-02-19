import os

import jinja2

template_name = "gtcomputation.cpp.in"

env = jinja2.Environment(loader=jinja2.FileSystemLoader('.'))
template = env.get_template(template_name)

stencil_definitions="""
struct diag_diff_1_functor {
    using out = gt::inout_accessor<0>;
    using in = gt::in_accessor<1, gt::extent<-1, 0, -1, 0>>;
    using param_list = gt::make_param_list<out, in>;

    template <typename Evaluation>
    GT_FUNCTION static void apply(Evaluation &eval) {
        eval(out()) = eval(in(-1, -1, 0));
    }
};
"""
computation_definition = """
gt::make_multistage(gt::enumtype::execute<gt::enumtype::forward>(),
                    gt::make_stage<diag_diff_1_functor>(
                        p_f_out(), p_f_in()))
"""

params = dict(module_name='gtcomputation',
              stencil_definitions=stencil_definitions,
              computation_definition=computation_definition,
              ndims=3,
              halo_size_i=1,
              halo_size_j=1,
              halo_size_k=0,
              level_offset_limit=1,
              input_params=[
                  (0, 'f_out', 'float'),
                  (1, 'f_in', 'double')])

with open(template_name[:-3], 'w') as f:
  f.write(template.render(params))
