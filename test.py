import gtcomputation as gtcomp_ijk
import gtcomputation_kji as gtcomp_kji
import gtcomputation_struct as gtcomp_struct
import gtboundary
import copy_simple
import numpy as np
import itertools

import pytest

def get_domain_masks(size, start, stop, halo):
    compute_domain = np.zeros(shape=size, dtype='bool')
    total_domain = np.zeros(shape=size, dtype='bool')

    compute_domain[start[0]:stop[0], start[1]:stop[1], start[2]:stop[2]] = True
    total_domain[start[0]-halo:stop[0]+halo, start[1]-halo:stop[1]+halo, start[2]:stop[2]] = True
    halo_domain = ~compute_domain & total_domain
    outer_domain = ~total_domain

    return compute_domain, halo_domain, outer_domain, total_domain

def create_numbered(shape, dtype, inversed = False):
    size = shape[0] * shape[1] * shape[2]
    if not inversed:
        return np.reshape(np.arange(start=0, stop=size, step=+1, dtype=dtype), shape)
    else:
        return np.reshape(np.arange(start=size, stop=0, step=-1, dtype=dtype), shape)

def test_copy_simple():
    fwd = create_numbered([30, 40, 50], np.double, inversed=False)
    bwd = create_numbered([30, 40, 50], np.double, inversed=True)

    f_in = fwd
    f_out = bwd

    copy_simple.copy(f_in, f_out)
    assert np.all(f_out == fwd)

@pytest.mark.parametrize("domain", [
    ([10, 20, 30]),
    ([5, 3, 8]),
    ([3, 5, 8]),
    ([3, 5, 1]),
])
def test_whole_domain(domain):
    calc_domain = domain
    f_in = create_numbered(domain, np.double, inversed=False)
    bwd = create_numbered(domain, np.double, inversed=True)
    f_out = create_numbered(domain, np.double, inversed=True)
    halo = 1

    comp = gtcomp_ijk.GTComputation(shape=calc_domain, halo=halo)
    comp.run(f_out=f_out, f_in=f_in)

    assert np.all(f_out[halo:-halo, halo:-halo, :] == f_in[halo-1:-halo-1, halo-1:-halo-1, :])
    assert np.all(f_out[:halo, :, :] == bwd[:halo, :, :])
    assert np.all(f_out[:, :halo, :] == bwd[:, :halo, :])
    assert np.all(f_out[-halo:, :, :] == bwd[-halo:, :, :])
    assert np.all(f_out[:, -halo:, :] == bwd[:, -halo:, :])

@pytest.mark.parametrize("domain,subdomain", [
    ([10, 20, 30], [10, 20, 30]),
    ([10, 20, 30], [5, 6, 7]),
    ([10, 20, 30], [3, 5, 7]),
    ([10, 20, 30], [5, 3, 7]),
    ([10, 20, 30], [5, 3, 1]),
])
def test_sub_domain_with_zero_origin(domain, subdomain):
    f_in = create_numbered(domain, np.double, inversed=False)
    bwd = create_numbered(domain, np.double, inversed=True)
    f_out = create_numbered(domain, np.double, inversed=True)
    halo = 1

    comp = gtcomp_ijk.GTComputation(shape=subdomain, halo=halo)
    comp.run(f_out=f_out, f_in=f_in)

    start = [halo, halo, 0]
    stop = [subdomain[0] - halo, subdomain[1] - halo, subdomain[2]]
    assert np.all(f_out[start[0]:stop[0], start[1]:stop[1], start[2]:stop[2]] \
                  == f_in[start[0]-1:stop[0]-1, start[1]-1:stop[1]-1, start[2]:stop[2]])
    assert np.all(f_out[:start[0], :, :] == bwd[:start[0], :, :])
    assert np.all(f_out[:, :start[1], :] == bwd[:, :start[1], :])
    assert np.all(f_out[:, :, :start[2]] == bwd[:, :, :start[2]])
    assert np.all(f_out[stop[0]:, :, :] == bwd[stop[0]:, :, :])
    assert np.all(f_out[:, stop[1]:, :] == bwd[:, stop[1]:, :])
    assert np.all(f_out[:, :, stop[2]:] == bwd[:, :, stop[2]:])

@pytest.mark.parametrize("domain,subdomain,origin", [
    ([10, 20, 30], [10, 20, 30], [0, 0, 0]),
    ([10, 20, 30], [5, 6, 7], [3, 4, 5]),
    ([10, 20, 30], [3, 5, 7], [7, 15, 23]),
    ([10, 20, 30], [5, 3, 7], [5, 1, 0]),
    ([10, 20, 30], [5, 3, 1], [2, 3, 1]),
])
def test_sub_domain_with_nonzero_origin_noshift(domain, subdomain, origin):
    f_in = create_numbered(domain, np.double, inversed=False)
    bwd = create_numbered(domain, np.double, inversed=True)
    f_out = create_numbered(domain, np.double, inversed=True)
    halo = 1

    comp = gtcomp_ijk.GTComputation(shape=subdomain, halo=halo)
    comp.run(f_out=f_out, f_in=f_in, f_out_origin=origin, f_in_origin=origin)

    start = [origin[0] + halo, origin[1] + halo, origin[2]]
    stop = [origin[0] + subdomain[0] - halo, origin[1] + subdomain[1] - halo, origin[2] + subdomain[2]]
    assert np.all(f_out[start[0]:stop[0], start[1]:stop[1], start[2]:stop[2]] \
                  == f_in[start[0]-1:stop[0]-1, start[1]-1:stop[1]-1, start[2]:stop[2]])
    assert np.all(f_out[:start[0], :, :] == bwd[:start[0], :, :])
    assert np.all(f_out[:, :start[1], :] == bwd[:, :start[1], :])
    assert np.all(f_out[:, :, :start[2]] == bwd[:, :, :start[2]])
    assert np.all(f_out[stop[0]:, :, :] == bwd[stop[0]:, :, :])
    assert np.all(f_out[:, stop[1]:, :] == bwd[:, stop[1]:, :])
    assert np.all(f_out[:, :, stop[2]:] == bwd[:, :, stop[2]:])

@pytest.mark.parametrize("domain,subdomain,origin_in,origin_out", [
    ([10, 20, 30], [5, 6, 7], [3, 4, 5], [5, 4, 3]),
    ([10, 20, 30], [5, 6, 7], [4, 3, 2], [3, 4, 5]),
])
def test_sub_domain_with_nonzero_origin_shift(domain, subdomain, origin_in, origin_out):
    f_in = create_numbered(domain, np.double, inversed=False)
    bwd = create_numbered(domain, np.double, inversed=True)
    f_out = create_numbered(domain, np.double, inversed=True)
    halo = 1

    comp = gtcomp_ijk.GTComputation(shape=subdomain, halo=halo)
    comp.run(f_out=f_out, f_in=f_in, f_out_origin=origin_out, f_in_origin=origin_in)

    start_in = [origin_in[0] + halo, origin_in[1] + halo, origin_in[2]]
    stop_in = [origin_in[0] + subdomain[0] - halo, origin_in[1] + subdomain[1] - halo, origin_in[2] + subdomain[2]]

    start_out = [origin_out[0] + halo, origin_out[1] + halo, origin_out[2]]
    stop_out = [origin_out[0] + subdomain[0] - halo, origin_out[1] + subdomain[1] - halo, origin_out[2] + subdomain[2]]
    assert np.all(f_out[start_out[0]:stop_out[0], start_out[1]:stop_out[1], start_out[2]:stop_out[2]] \
                  == f_in[start_in[0]-1:stop_in[0]-1, start_in[1]-1:stop_in[1]-1, start_in[2]:stop_in[2]])
    assert np.all(f_out[:start_out[0], :, :] == bwd[:start_out[0], :, :])
    assert np.all(f_out[:, :start_out[1], :] == bwd[:, :start_out[1], :])
    assert np.all(f_out[:, :, :start_out[2]] == bwd[:, :, :start_out[2]])
    assert np.all(f_out[stop_out[0]:, :, :] == bwd[stop_out[0]:, :, :])
    assert np.all(f_out[:, stop_out[1]:, :] == bwd[:, stop_out[1]:, :])
    assert np.all(f_out[:, :, stop_out[2]:] == bwd[:, :, stop_out[2]:])

@pytest.mark.parametrize("domain", [
    ([10, 4, 5]),
    ([5, 3, 8]),
    ([3, 5, 8]),
    ([3, 5, 1]),
])
def test_whole_domain_with_kji(domain):
    calc_domain = domain
    f_in = create_numbered(domain, np.double, inversed=False)
    bwd = create_numbered(domain, np.double, inversed=True)
    f_out = create_numbered(domain, np.double, inversed=True)
    f_in = np.copy(f_in, 'F')
    bwd = np.copy(bwd, 'F')
    f_out = np.copy(f_out, 'F')
    halo = 1

    comp = gtcomp_kji.GTComputation(shape=calc_domain, halo=halo)
    comp.run(f_out=f_out, f_in=f_in)

    assert np.all(f_out[halo:-halo, halo:-halo, :] == f_in[halo-1:-halo-1, halo-1:-halo-1, :])
    assert np.all(f_out[:halo, :, :] == bwd[:halo, :, :])
    assert np.all(f_out[:, :halo, :] == bwd[:, :halo, :])
    assert np.all(f_out[-halo:, :, :] == bwd[-halo:, :, :])
    assert np.all(f_out[:, -halo:, :] == bwd[:, -halo:, :])

@pytest.mark.parametrize("domain,subdomain,origin_in,origin_out", [
    ([10, 20, 30], [5, 6, 7], [3, 4, 5], [5, 4, 3]),
    ([10, 20, 30], [5, 6, 7], [4, 3, 2], [3, 4, 5]),
])
def test_sub_domain_with_nonzero_origin_shift_with_kji(domain, subdomain, origin_in, origin_out):
    f_in = create_numbered(domain, np.double, inversed=False)
    bwd = create_numbered(domain, np.double, inversed=True)
    f_out = create_numbered(domain, np.double, inversed=True)
    f_in = np.copy(f_in, 'F')
    bwd = np.copy(bwd, 'F')
    f_out = np.copy(f_out, 'F')
    halo = 1

    comp = gtcomp_kji.GTComputation(shape=subdomain, halo=halo)
    comp.run(f_out=f_out, f_in=f_in, f_out_origin=origin_out, f_in_origin=origin_in)

    start_in = [origin_in[0] + halo, origin_in[1] + halo, origin_in[2]]
    stop_in = [origin_in[0] + subdomain[0] - halo, origin_in[1] + subdomain[1] - halo, origin_in[2] + subdomain[2]]

    start_out = [origin_out[0] + halo, origin_out[1] + halo, origin_out[2]]
    stop_out = [origin_out[0] + subdomain[0] - halo, origin_out[1] + subdomain[1] - halo, origin_out[2] + subdomain[2]]

    assert np.all(f_out[start_out[0]:stop_out[0], start_out[1]:stop_out[1], start_out[2]:stop_out[2]] \
                  == f_in[start_in[0]-1:stop_in[0]-1, start_in[1]-1:stop_in[1]-1, start_in[2]:stop_in[2]])
    assert np.all(f_out[:start_out[0], :, :] == bwd[:start_out[0], :, :])
    assert np.all(f_out[:, :start_out[1], :] == bwd[:, :start_out[1], :])
    assert np.all(f_out[:, :, :start_out[2]] == bwd[:, :, :start_out[2]])
    assert np.all(f_out[stop_out[0]:, :, :] == bwd[stop_out[0]:, :, :])
    assert np.all(f_out[:, stop_out[1]:, :] == bwd[:, stop_out[1]:, :])
    assert np.all(f_out[:, :, stop_out[2]:] == bwd[:, :, stop_out[2]:])

@pytest.mark.parametrize("domain,subdomain,origin_in,origin_out", [
    ([10, 20, 30], [5, 6, 7], [3, 4, 5], [5, 4, 3]),
    ([10, 20, 30], [5, 6, 7], [4, 3, 2], [3, 4, 5]),
])
def test_boundary_simple(domain, subdomain, origin_in, origin_out):
    fwd = create_numbered(domain, np.double, inversed=False)
    bwd = create_numbered(domain, np.double, inversed=True)

    f_in = create_numbered(domain, np.double, inversed=False)
    f_out = create_numbered(domain, np.double, inversed=True)
    halo = 1

    boundary = gtboundary.GTCopyBoundary(shape=subdomain, halo=halo)
    boundary.run(f_out=f_out, f_in=f_in, f_out_origin=origin_out, f_in_origin=origin_in)

    start_in = [origin_in[0] + halo, origin_in[1] + halo, origin_in[2]]
    stop_in = [origin_in[0] + subdomain[0] - halo, origin_in[1] + subdomain[1] - halo, origin_in[2] + subdomain[2]]

    start_out = [origin_out[0] + halo, origin_out[1] + halo, origin_out[2]]
    stop_out = [origin_out[0] + subdomain[0] - halo, origin_out[1] + subdomain[1] - halo, origin_out[2] + subdomain[2]]

    in_compute_domain, in_halo_domain, in_outer_domain, in_total_domain = get_domain_masks(f_in.shape, start_in, stop_in, halo)
    out_compute_domain, out_halo_domain, out_outer_domain, out_total_domain = get_domain_masks(f_out.shape, start_out, stop_out, halo)

    assert np.all(f_in == fwd)

    assert np.all(f_out[np.where(out_compute_domain)] == bwd[np.where(out_compute_domain)])
    assert np.all(f_out[np.where(out_outer_domain)] == bwd[np.where(out_outer_domain)])
    assert np.all(f_out[np.where(out_halo_domain)] == fwd[np.where(in_halo_domain)])

@pytest.mark.parametrize("domain", [
    ([10, 20, 30]),
    ([5, 3, 8]),
    ([3, 5, 8]),
    ([3, 5, 1]),
])
def test_computation_struct(domain):

    dtype = np.dtype([('x', np.float64), ('y', np.float64)])
    halo = 1

    calc_domain = domain
    f_in = np.zeros(domain, dtype=dtype)
    f_ref = np.zeros(domain, dtype=dtype)
    f_out = np.zeros(domain, dtype=dtype)
    for i, j, k in itertools.product(range(domain[0]), range(domain[1]), range(domain[2])):
        if i < halo or i >= calc_domain[0] - 1 or j < halo or j >= calc_domain[1] - 1:
            print (i, j)
            f_ref[i, j, k]['x'] = f_out[i, j, k]['x'] = 2 * i - j + k
            f_ref[i, j, k]['y'] = f_out[i, j, k]['y'] = 2 * i + j + k
        else:
            f_in[i, j, k]['y'] = i - j + k
            f_in[i, j, k]['x'] = i + j + k
            f_ref[i, j, k]['x'] = i - j + k + 1.0
            f_ref[i, j, k]['y'] = i + j + k + 2.0

    comp = gtcomp_struct.GTComputationStruct(shape=calc_domain, halo=halo)
    comp.run(f_out=f_out, f_in=f_in, x=1, y=2)

    assert np.all(f_out[halo:-halo, halo:-halo, :] == f_ref[halo:-halo, halo:-halo, :])
    assert np.all(f_out[:halo, :, :] == f_ref[:halo, :, :])
    assert np.all(f_out[:, :halo, :] == f_ref[:, :halo, :])
    assert np.all(f_out[-halo:, :, :] == f_ref[-halo:, :, :])
    assert np.all(f_out[:, -halo:, :] == f_ref[:, -halo:, :])
