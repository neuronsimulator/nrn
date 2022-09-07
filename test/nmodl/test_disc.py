from disc import DiscCell

def test_disc():
    disc_cell = DiscCell()
    disc_cell.create_cell()
    disc_cell.record()
    disc_cell.simulate(1, 0.1)
    
    print(disc_cell["a"])