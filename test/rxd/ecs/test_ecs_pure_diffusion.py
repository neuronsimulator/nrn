import pytest

from testutils import compare_data, tol


@pytest.fixture
def ecs_diffusion(neuron_instance):
    h, rxd, data, save_path = neuron_instance

    def make_model(nx, ny, nz, alpha, lambd, d=1.0, perm=None):
        if len(list(h.allsec())) == 0:
            # Only needed to gather data.
            # Persists until cleaned up by neuron_nosave_instance
            h("create dummy")
        dx = 10
        # the extracellular space
        ecs = rxd.Extracellular(
            -dx * nx / 2.0,
            -dx * ny / 2.0,
            -dx * nz / 2.0,
            dx * nx / 2.0,
            dx * ny / 2.0,
            dx * nz / 2.0,
            dx=dx,
            volume_fraction=alpha,
            tortuosity=lambd,
            permeability=perm,
        )

        # Who?
        k = rxd.Species(
            [ecs],
            name="k",
            d=d,
            charge=1,
            initial=lambda nd: (
                1 if nd.x3d**2 + nd.y3d**2 + nd.z3d**2 < (2 * dx) ** 2 else 0
            ),
        )

        model = (ecs, k)
        return model

    yield (neuron_instance, make_model)


def test_ecs_diffusion_2d(ecs_diffusion):
    neuron_instance, make_model = ecs_diffusion
    ecs, k = make_model(11, 11, 1, 0.2, 1.6, 0)
    h, rxd, data, save_path = neuron_instance
    h.finitialize(1000)
    k.d = 1
    h.finitialize(1000)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_diffusion_2d_cvode(ecs_diffusion):
    neuron_instance, make_model = ecs_diffusion
    ecs, k = make_model(11, 11, 1, 0.2, 1.6, 0)
    h, rxd, data, save_path = neuron_instance
    h.CVode().active(True)
    h.finitialize(1000)
    k.d = 1
    h.finitialize(1000)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_diffusion_1d(ecs_diffusion):
    neuron_instance, make_model = ecs_diffusion
    ecs, k = make_model(1, 11, 1, 0.2, 1.6, 0)
    h, rxd, data, save_path = neuron_instance
    h.finitialize(1000)
    k.nodes[0].d = 1.0
    h.finitialize(1000)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_diffusion_1d_cvode(ecs_diffusion):
    neuron_instance, make_model = ecs_diffusion
    ecs, k = make_model(1, 11, 1, 0.2, 1.6, 0)
    h, rxd, data, save_path = neuron_instance
    h.CVode().active(True)
    h.finitialize(1000)
    k[ecs].d = (1, 1, 1)
    h.finitialize(1000)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_diffusion_0d(ecs_diffusion):
    neuron_instance, make_model = ecs_diffusion
    ecs, k = make_model(1, 1, 1, 0.2, 1.6, 0)
    h, rxd, data, save_path = neuron_instance
    h.finitialize(1000)
    h.continuerun(10)
    if not save_path:
        assert k[ecs].states3d[0] == 1.0


def test_ecs_diffusion_0d_cvode(ecs_diffusion):
    neuron_instance, make_model = ecs_diffusion
    h, rxd, data, save_path = neuron_instance
    ecs, k = make_model(1, 1, 1, 0.2, 1.6, 0)
    h.CVode().active(True)
    h.finitialize(1000)
    h.continuerun(10)
    if not save_path:
        assert k[ecs].states3d[0] == 1.0


def test_ecs_diffusion_2d_tort(ecs_diffusion):
    neuron_instance, make_model = ecs_diffusion
    ecs, k = make_model(11, 11, 1, 0.2, lambda x, y, z: 1.6, 0)
    h, rxd, data, save_path = neuron_instance
    h.finitialize(1000)
    k[ecs].d = (1, 1, 1)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_diffusion_2d_cvode_tort(ecs_diffusion):
    neuron_instance, make_model = ecs_diffusion
    ecs, k = make_model(11, 11, 1, 0.2, lambda x, y, z: 1.6, 0)
    h, rxd, data, save_path = neuron_instance
    h.CVode().active(True)
    h.finitialize(1000)
    k.nodes[0].d = (1, 1, 1)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_diffusion_1d_tort(ecs_diffusion):
    neuron_instance, make_model = ecs_diffusion
    ecs, k = make_model(1, 11, 1, 0.2, lambda x, y, z: 1.6)
    h, rxd, data, save_path = neuron_instance
    h.finitialize(1000)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_diffusion_1d_cvode_tort(ecs_diffusion):
    neuron_instance, make_model = ecs_diffusion
    model = make_model(1, 11, 1, 0.2, lambda x, y, z: 1.6)
    h, rxd, data, save_path = neuron_instance
    h.CVode().active(True)
    h.finitialize(1000)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_diffusion_0d_tort(ecs_diffusion):
    neuron_instance, make_model = ecs_diffusion
    ecs, k = make_model(1, 1, 1, 0.2, lambda x, y, z: 1.6)
    h, rxd, data, save_path = neuron_instance
    h.finitialize(1000)
    h.continuerun(10)
    if not save_path:
        assert k[ecs].states3d[0] == 1.0


def test_ecs_diffusion_0d_cvode_tort(ecs_diffusion):
    neuron_instance, make_model = ecs_diffusion
    h, rxd, data, save_path = neuron_instance
    ecs, k = make_model(1, 1, 1, 0.2, lambda x, y, z: 1.6)
    h.CVode().active(True)
    h.finitialize(1000)
    h.continuerun(10)
    if not save_path:
        assert k[ecs].states3d[0] == 1.0


def test_ecs_diffusion_2d_alpha(ecs_diffusion):
    neuron_instance, make_model = ecs_diffusion
    model = make_model(11, 11, 1, lambda x, y, z: 0.2, 1.6)
    h, rxd, data, save_path = neuron_instance
    h.finitialize(1000)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_diffusion_2d_cvode_alpha(ecs_diffusion):
    neuron_instance, make_model = ecs_diffusion
    model = make_model(11, 11, 1, lambda x, y, z: 0.2, 1.6)
    h, rxd, data, save_path = neuron_instance
    h.CVode().active(True)
    h.finitialize(1000)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_diffusion_1d_alpha(ecs_diffusion):
    neuron_instance, make_model = ecs_diffusion
    model = make_model(1, 11, 1, lambda x, y, z: 0.2, 1.6)
    h, rxd, data, save_path = neuron_instance
    h.finitialize(1000)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_diffusion_1d_cvode_alpha(ecs_diffusion):
    neuron_instance, make_model = ecs_diffusion
    model = make_model(1, 11, 1, lambda x, y, z: 0.2, 1.6)
    h, rxd, data, save_path = neuron_instance
    h.CVode().active(True)
    h.finitialize(1000)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_diffusion_0d_alpha(ecs_diffusion):
    neuron_instance, make_model = ecs_diffusion
    model = make_model(1, 1, 1, lambda x, y, z: 0.2, 1.6)
    h, rxd, data, save_path = neuron_instance
    h.finitialize(1000)
    h.continuerun(10)
    ecs, k = model
    if not save_path:
        assert k[ecs].states3d[0] == 1.0


def test_ecs_diffusion_0d_cvode_alpha(ecs_diffusion):
    neuron_instance, make_model = ecs_diffusion
    h, rxd, data, save_path = neuron_instance
    model = make_model(1, 1, 1, lambda x, y, z: 0.2, 1.6)
    h.CVode().active(True)
    h.finitialize(1000)
    h.continuerun(10)
    ecs, k = model
    if not save_path:
        assert k[ecs].states3d[0] == 1.0


def test_ecs_diffusion_anisotropic(ecs_diffusion):
    neuron_instance, make_model = ecs_diffusion
    model = make_model(11, 11, 11, 0.2, 1.6, d=(0.25, 0.5, 1.0))
    h, rxd, data, save_path = neuron_instance
    h.CVode().active(True)
    h.finitialize(1000)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_diffusion_cvode_anisotropic(ecs_diffusion):
    neuron_instance, make_model = ecs_diffusion
    model = make_model(11, 11, 11, 0.2, 1.6, d=(0.25, 0.5, 1.0))
    h, rxd, data, save_path = neuron_instance
    h.finitialize(1000)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_diffusion_3d_tort_state(ecs_diffusion):
    neuron_instance, make_model = ecs_diffusion
    ecs, k = make_model(
        11, 11, 11, 0.2, None, perm={"name": "permeability", "initial": 0}
    )
    h, rxd, data, save_path = neuron_instance
    for nd in ecs.permeability.nodes:
        nd.value = 100 / (10 + (nd.x3d + nd.y3d + nd.z3d) ** 2)
    h.finitialize(1000)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_diffusion_3d_inhom_tort(ecs_diffusion):
    neuron_instance, make_model = ecs_diffusion
    ecs, k = make_model(
        11, 11, 11, 0.2, None, perm=lambda x, y, z: 100 / (10 + (x + y + z) ** 2)
    )
    h, rxd, data, save_path = neuron_instance
    h.finitialize(1000)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_diffusion_3d_inhom_tort_cvode(ecs_diffusion):
    neuron_instance, make_model = ecs_diffusion
    ecs, k = make_model(
        11, 11, 11, 0.2, None, perm=lambda x, y, z: 100 / (10 + (x + y + z) ** 2)
    )
    h, rxd, data, save_path = neuron_instance
    h.CVode().active(True)
    h.finitialize(1000)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_diffusion_3d_alpha_state(ecs_diffusion):
    neuron_instance, make_model = ecs_diffusion
    ecs, k = make_model(11, 11, 11, {"name": "alpha", "initial": 1}, 1.6)
    h, rxd, data, save_path = neuron_instance
    h.finitialize(1000)
    ecs.alpha.nodes.value = 0.2
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol
