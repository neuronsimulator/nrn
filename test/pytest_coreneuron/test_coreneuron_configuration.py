from neuron import coreneuron


def test_coreneuron_configuration():
    with coreneuron(
        restore_path="restore",
        save_path="save",
        model_path="coredat",
        skip_write_model_to_disk=True,
    ):
        assert coreneuron.restore_path == "restore"
        assert coreneuron.save_path == "save"
        assert coreneuron.model_path == "coredat"
        assert coreneuron.skip_write_model_to_disk == True

    #  back to the old values outside the "with" context
    assert coreneuron.restore_path is None
    assert coreneuron.save_path is None
    assert coreneuron.model_path is None
    assert coreneuron.skip_write_model_to_disk == False


if __name__ == "__main__":
    test_coreneuron_configuration()
