#!/usr/bin/python

storeDelimetter = "Store"
loadDelimetter  = "Load"

import dbiSimTests

def isNumber(num):
    try:
        int(num)
        return True
    except ValueError:
        return False

def readFile(filename):
    lines = []
    with open(filename, 'r') as f:
        lines = f.readlines()

    return lines

def stringContains(fullString, subString):
    if fullString.find(subString) == -1:
        return False
    else:
        return True

def filterMisses(lines):
    filteredLines = []
    for line in lines:
        if (stringContains(line, storeDelimetter)):
            filteredLines.append(line)
        elif (stringContains(line, loadDelimetter)):
            filteredLines.append(line)

    return filteredLines

def copyDelimmtedLines(lines, delimmeter):
    filteredLines = []
    for line in lines:
        if (stringContains(line, delimmeter)):
            filteredLines.append(line)

    return filteredLines

def getAddr(miss):
    missSplit = miss.split(' ')
    try :
        addrIndex = missSplit.index('Addr')
        return int(missSplit[addrIndex + 2])
    except ValueError:
        return "None"

def getValue(miss):
    missSplit = miss.split(' ')
    try :
        valIndex = missSplit.index('value')
        return int(missSplit[valIndex + 2].rstrip('\n'))
    except ValueError:
        return "None"

def makeMissList(misses):
    missList = []
    for miss in misses:
        addr = getAddr(miss)
        value = getValue(miss)
        if isNumber(addr) and isNumber(value):
            missList.append([abs(addr), abs(value)])

    return missList

def countOnes(num, bitwidth):
    numOnes = 0
    for i in num:
        numOnes += int(i)

    return numOnes

def countZeros(num, bitwidth):
    return bitwidth - countOnes(num, bitwidth)

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
    ones = countOnes(binaryDifference, bitwdith)
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


def printAddrs(misses):
    for (addr, value) in misses:
        print format(addr, '032b'), addr

def main():
    simNames = readFile("simulations")
    simNames = map(lambda x: x.rstrip('\n'), simNames)

    for simName in simNames:
        lines = readFile("memory-traces/" + simName)

        misses = filterMisses(lines)
        stores = copyDelimmtedLines(misses, storeDelimetter)
        loads = copyDelimmtedLines(misses, loadDelimetter)
        stores = makeMissList(stores)
        loads = makeMissList(loads)
        print simName + " Load AC  : " , loadDbiAc(loads, 32)
        print simName + " Store AC : ", storeDbiAc(stores, 32)
        print simName + " Load DC  : ", dbiDc(loads, 32)
        print simName + " Store DC : ", dbiDc(stores, 32)

if __name__ == '__main__':
    main()
