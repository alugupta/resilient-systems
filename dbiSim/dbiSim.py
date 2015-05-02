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
            missList.append([addr, value])

    return missList

def printAddrs(misses):
    for (addr, value) in misses:
        print format(addr, '016b'), addr

def main():
    dbiSimTests.test_isNumber()
    lines = readFile("basicmathOut")

    misses = filterMisses(lines)
    stores = copyDelimmtedLines(misses, storeDelimetter)
    loads = copyDelimmtedLines(misses, loadDelimetter)
    stores = makeMissList(stores)
    loads = makeMissList(loads)

    printAddrs(stores)

if __name__ == '__main__':
    main()
