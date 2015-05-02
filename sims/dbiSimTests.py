import dbiSim

def test_isNumber():
    assert(True  == dbiSim.isNumber(1))
    assert(True  == dbiSim.isNumber("1"))
    assert(True  == dbiSim.isNumber("-1"))
    assert(False == dbiSim.isNumber("xxx"))
    assert(True  == dbiSim.isNumber("0"))

def test_countOnes():
    assert( 1  == dbiSim.countOnes(1, 32))
    assert( 0  == dbiSim.countOnes(0, 32))
    assert( 1  == dbiSim.countOnes(2, 32))
    assert( 2  == dbiSim.countOnes(3, 32))
    assert( 1  == dbiSim.countOnes(4, 32))
    assert( 31  == dbiSim.countOnes( (2 ** 31)- 1, 32))
