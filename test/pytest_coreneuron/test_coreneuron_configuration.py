from neuron import coreneuron


def test_coreneuron_configuration():
    assert coreneuron.restore_path is None
    assert coreneuron.save_path is None
    assert coreneuron.model_path is None
    assert coreneuron.skip_write_model_to_disk == False

    coreneuron.restore_path = "restore"
    coreneuron.save_path = "save"
    coreneuron.model_path = "coredat"
    coreneuron.skip_write_model_to_disk = True

    assert coreneuron.restore_path == "restore"
    assert coreneuron.save_path == "save"
    assert coreneuron.model_path == "coredat"
    assert coreneuron.skip_write_model_to_disk == True


if __name__ == "__main__":
    test_coreneuron_configuration()
