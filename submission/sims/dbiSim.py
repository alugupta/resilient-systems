#!/usr/bin/python

storeDelimetter = "Store"
loadDelimetter  = "Load"

import dbiSimTests

################################################################################
# Checks to see if ``num'' can be converted to a valid integer.
# @returns : true if int(num) passes without ValueError exceptions, else false.
################################################################################
def isNumber(num):
    try:
        int(num)
        return True
    except ValueError:
        return False

################################################################################
# Reads all lines in a file
# @returns : a list of strings where the N-th string corresponds to the N-th
# line in ```filename'''
################################################################################
def readFile(filename):
    lines = []
    with open(filename, 'r') as f:
        lines = f.readlines()

    return lines

def writeFile(filename, lines):
    with open(filename, 'w') as f:
        f.writelines(lines)


################################################################################
# Checks to see is ``fullString'' contains ``substring''
# @args : fullString and substring must be strings
# @returns: true if fullString.find(substring) != -1
################################################################################
def stringContains(fullString, subString):
    if fullString.find(subString) == -1:
        return False
    else:
        return True

################################################################################
# Checks to see is ``fullString'' contains ``substring''
# @args : fullString and substring must be strings
# @returns: true if fullString.find(substring) != -1
################################################################################
def filterMisses(lines):
    filteredLines = []
    for line in lines:
        if (stringContains(line, storeDelimetter)):
            filteredLines.append(line)
        elif (stringContains(line, loadDelimetter)):
            filteredLines.append(line)

    return filteredLines

################################################################################
# Copies ``lines'' containing the ``delimmeter'' into a new list.
# @args: lines is a list of strings
# @args: delimmeter is a string
# @returns : a list containing all ``lines'' containing ``delimmeter''. The
# order is preserved.
################################################################################
def copyDelimmtedLines(lines, delimmeter):
    filteredLines = []
    for line in lines:
        if (stringContains(line, delimmeter)):
            filteredLines.append(line)

    return filteredLines

################################################################################
# Returns the address corresponding to the ``miss'' provided. This assumes that
# the ``miss'' is encoding as :
#           ----some-text----_'Addr'_*_'addr-value'_-----some-text
# where the _ are actual spaces, and * is any string without spaces and
# 'addr-value' is the desired address value
# @returns : the int corresponding to the address if ``miss'' is a valid miss
# and the string "None" otherwise
################################################################################
def getAddr(miss):
    missSplit = miss.split(' ')
    try :
        addrIndex = missSplit.index('Addr')
        return int(missSplit[addrIndex + 2])
    except ValueError:
        return "None"

################################################################################
# Returns the data value corresponding to the ``miss'' provided. This assumes
# that the ``miss'' is encoding as :
#           ----some-text----_'value'_*_'data-value'_-----some-text
# where the _ are actual spaces, and * is any string without spaces and
# 'data-value' is the desired value
# @returns : the int corresponding to the data-value if ``miss'' is a valid miss
# and the string "None" otherwise
################################################################################
def getValue(miss):
    missSplit = miss.split(' ')
    try :
        valIndex = missSplit.index('value')
        return int(missSplit[valIndex + 2].rstrip('\n'))
    except ValueError:
        return "None"

################################################################################
# Returns a sequence of misses addresses and data-values given the list of
# encoded ```misses'''
# that the ``miss'' is encoding as :
#
#----some-text----__'Addr'_*_ 'addr-value'_*_'value'_*_'data-value'_---some-text
#
# where the _ are actual spaces, and * is any string without spaces and
# 'data-value' is the desired value and 'addr-value' is the desired address.
#
# @returns : a list containing lists of : [address, value] pairs. Order from
# the encoded ``misses'' is preserved
################################################################################
def makeMissList(misses):
    missList = []
    for miss in misses:
        addr = getAddr(miss)
        value = getValue(miss)
        if isNumber(addr) and isNumber(value):
            missList.append([abs(addr), abs(value)])

    return missList

################################################################################
# Population count on ``num''.
# @args: num a binary string
# @returns: the number of 1's in the binary string, ``num''.
################################################################################
def countOnes(num):
    numOnes = 0
    for i in num:
        numOnes += int(i)

    return numOnes

################################################################################
# Counts the number of zeros in ``num''.
# @args: num a binary string
# @returns: bitwdith - countOnes(num)
################################################################################
def countZeros(num, bitwidth):
    return bitwidth - countOnes(num)

################################################################################
# Simulates system with no DBI.
################################################################################
def nonSingleDbiDc(num, bitwidth):
    binaryNum = format(num, '0' + str(bitwidth) + 'b')
    byteNum = [binaryNum[0:8], binaryNum[8:16], binaryNum[16:24], binaryNum[24:32]]
    zeros = map(lambda x: countZeros(x, 8), byteNum)
    totalZeros = 0
    for count in zeros:
        totalZeros += count

    return totalZeros

def noncumulativeDbiDc(nums, bitwdith):
    totalZeros = 0
    totalBits = len(nums) * bitwdith
    for num in nums:
        totalZeros += nonSingleDbiDc(num, bitwdith)

    return float(totalZeros) / float(totalBits)

def nondbiDc(misses, bitwdith):
    _, values = zip(*misses)
    return noncumulativeDbiDc(values, bitwdith)

def nonDbiAc(previous, new, bitwdith):
    difference = previous ^ new
    binaryDifference = format(difference, '0' + str(bitwdith) + 'b')
    return countOnes(binaryDifference)

def nonStoreAc(stores, bitwdith):
    seenAddresses = []
    seenValues = []

    totalDbiCount = 0
    totalBits = len(stores) * bitwdith
    for [addr, value] in stores:
        try:
            index = seenAddresses.index(addr)
            totalDbiCount += nonDbiAc(seenValues[index], value, bitwdith)
        except ValueError:
            totalDbiCount += nonDbiAc(0, value, bitwdith)

        seenAddresses.insert(0, addr)
        seenValues.insert(0, addr)

    return float(totalDbiCount) / float(totalBits)

def nonLoadDbiAc(loads, bitwdith):
    _, values = zip(*loads)
    previous = 0
    totalDbiCount = 0
    totalBits = len(values) * bitwdith
    for value in values:
        totalDbiCount += nonDbiAc(previous, value, bitwdith)
        previous = value

    return float(totalDbiCount) / float(totalBits)

################################################################################
# Simulates the zero sensitive DBI-DC algorithm for a single binary string,
################################################################################
def singleDbiDc(num, bitwdith):
    binaryNum = format(num, '0' + str(bitwdith) + 'b')
    byteNum = [binaryNum[0:8], binaryNum[8:16], binaryNum[16:24], binaryNum[24:32]]
    zeros = map(lambda x: countZeros(x, 8), byteNum)
    totalZeros = 0
    for count in zeros:
        if (count > 4):
            totalZeros += (8 - count)
        else :
            totalZeros += count
    return totalZeros

def cumulativeDbiDc(nums, bitwdith):
    totalZeros = 0
    totalBits = len(nums) * bitwdith
    for num in nums:
        totalZeros += singleDbiDc(num, bitwdith)

    return float(totalZeros) / float(totalBits)

def singleDbiAc(previous, new, bitwdith):
    difference = previous ^ new
    binaryDifference = format(difference, '0' + str(bitwdith) + 'b')
    ones = countOnes(binaryDifference)
    if (ones > 16):
        return bitwdith - ones
    else:
        return ones

def dbiDc(misses, bitwdith):
    _, values = zip(*misses)
    return cumulativeDbiDc(values, bitwdith)


def storeDbiAc(stores, bitwdith):
    seenAddresses = []
    seenValues = []

    totalDbiCount = 0
    totalBits = len(stores) * bitwdith
    for [addr, value] in stores:
        try:
            index = seenAddresses.index(addr)
            totalDbiCount += singleDbiAc(seenValues[index], value, bitwdith)
        except ValueError:
            totalDbiCount += singleDbiAc(0, value, bitwdith)

        seenAddresses.insert(0, addr)
        seenValues.insert(0, addr)

    return float(totalDbiCount) / float(totalBits)

def loadDbiAc(loads, bitwdith):
    _, values = zip(*loads)
    previous = 0
    totalDbiCount = 0
    totalBits = len(values) * bitwdith
    for value in values:
        totalDbiCount += singleDbiAc(previous, value, bitwdith)
        previous = value

    return float(totalDbiCount) / float(totalBits)

def comparitiveAnalysis(loaddc, loadac, storedc, storeac, nonloaddc, nonloadac,
        nonstoredc, nonstoreac):

    loaddcDBI  = map(lambda l, n: (float(l) - float(n) ) / float(n) , loaddc, nonloaddc)
    loadacDBI  = map(lambda l, n: (float(l) - float(n) ) / float(n) , loadac, nonloadac)
    storedcDBI = map(lambda l, n: (float(l) - float(n) ) / float(n) , storedc, nonstoredc)
    storeacDBI = map(lambda l, n: (float(l) - float(n) ) / float(n) , storeac, nonstoreac)

    elements   = len(loadac)
    loaddcDBI  = abs(reduce(lambda x, y: x + y, loaddcDBI, 0) / elements * 100)
    loadacDBI  = abs(reduce(lambda x, y: x + y, loadacDBI, 0) / elements * 100)
    storedcDBI = abs(reduce(lambda x, y: x + y, storedcDBI, 0) / elements * 100)
    storeacDBI = abs(reduce(lambda x, y: x + y, storeacDBI, 0) / elements * 100)

    print loaddcDBI
    print loadacDBI
    print storedcDBI
    print storeacDBI

def main():
    simNames = readFile("simulations")
    simNames = map(lambda x: x.rstrip('\n'), simNames)

    nonloadac  = []
    nonstoreac = []
    nonloaddc  = []
    nonstoredc = []

    loadac  = []
    storeac = []
    loaddc  = []
    storedc = []

    for simName in simNames:
        lines = readFile("memory-traces/" + simName)

        misses = filterMisses(lines)
        stores = copyDelimmtedLines(misses, storeDelimetter)
        loads = copyDelimmtedLines(misses, loadDelimetter)
        stores = makeMissList(stores)
        loads = makeMissList(loads)

        loadac.append(str(loadDbiAc(loads,    32 )) + '\n')
        storeac.append(str(storeDbiAc(stores, 32 )) + '\n')
        loaddc.append(str(dbiDc(loads,        32 )) + '\n')
        storedc.append(str(dbiDc(stores,      32 )) + '\n')

        nonloadac.append(str (nonLoadDbiAc(loads, 32 )) + '\n')
        nonstoreac.append(str(nonStoreAc(stores,  32 )) + '\n')
        nonloaddc.append(str (nondbiDc(loads,     32 )) + '\n')
        nonstoredc.append(str(nondbiDc(stores,    32 )) + '\n')

    writeFile("plots/loaddc",  loaddc)
    writeFile("plots/loadac",  loadac)
    writeFile("plots/storeac", storeac)
    writeFile("plots/storedc", storedc)

    writeFile("plots/nonloaddc",  nonloaddc)
    writeFile("plots/nonloadac",  nonloadac)
    writeFile("plots/nonstoreac", nonstoreac)
    writeFile("plots/nonstoredc", nonstoredc)

    comparitiveAnalysis(loaddc, loadac, storedc, storeac, nonloaddc, nonloadac,
        nonstoredc, nonstoreac)

if __name__ == '__main__':
    main()