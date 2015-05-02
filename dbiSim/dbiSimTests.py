import dbiSim

def test_isNumber():
    assert(True  == dbiSim.isNumber(1))
    assert(True  == dbiSim.isNumber("1"))
    assert(True  == dbiSim.isNumber("-1"))
    assert(False == dbiSim.isNumber("xxx"))
    assert(True  == dbiSim.isNumber("0"))
