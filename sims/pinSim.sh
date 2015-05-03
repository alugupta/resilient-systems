#!/bin/bash

#sudo pin -t dcache.so -c 32 -a 8 -b 128
###############################################################################
# SIMULATION DIR
###############################################################################
CWD="/home/udit/git/resilient-systems/sims"

###############################################################################
# PIN ENV
###############################################################################
PIN_DIR="/home/udit/git/resilient-systems/pin/"
PIN_TOOL="$PIN_DIR/pin"
DCACHE="$PIN_DIR/source/tools/Memory/obj-ia32/dcache.so"

###############################################################################
# TRACE OUTPUT FOLDER
###############################################################################
TRACES="$CWD/memory-traces"

###############################################################################
# MI-BENCH APPS
###############################################################################
MI_BENCH="/home/udit/git/resilient-systems/benchmarks/mibench"

AUTOMOTIVE="automotive"
CONSUMER="consumer"
NETWORK="network"
OFFICE="office"
SECURITY="security"
TELECOMM="telecomm"

rm -rf $TRACES
mkdir $TRACES
################################################################################
# MI-BENCH SIMS
################################################################################
sudo $PIN_TOOL -t $DCACHE -- $MI_BENCH/$AUTOMOTIVE/basicmath/basicmath_large >> $TRACES/$AUTOMOTIVE-basicmath-large
sudo $PIN_TOOL -t $DCACHE -- $MI_BENCH/$AUTOMOTIVE/basicmath/basicmath_small >> $TRACES/$AUTOMOTIVE-basicmath-small

cd $MI_BENCH/$AUTOMOTIVE/bitcount
sudo $PIN_TOOL -t $DCACHE -- ./runme_large.sh >> $TRACES/$AUTOMOTIVE-bitcount-large
sudo $PIN_TOOL -t $DCACHE -- ./runme_small.sh >> $TRACES/$AUTOMOTIVE-bitcount-small

cd $MI_BENCH/$AUTOMOTIVE/qsort
sudo $PIN_TOOL -t $DCACHE -- ./runme_large.sh >> $TRACES/$AUTOMOTIVE-qsort-large
sudo $PIN_TOOL -t $DCACHE -- ./runme_small.sh >> $TRACES/$AUTOMOTIVE-qsort-small

cd $MI_BENCH/$AUTOMOTIVE/susan
sudo $PIN_TOOL -t $DCACHE -- ./runme_large.sh >> $TRACES/$AUTOMOTIVE-susan-large
sudo $PIN_TOOL -t $DCACHE -- ./runme_small.sh >> $TRACES/$AUTOMOTIVE-susan-small

cd $MI_BENCH/$CONSUMER/jpeg
sudo $PIN_TOOL -t $DCACHE -- ./runme_large.sh >> $TRACES/$CONSUMER-jpeg-large
sudo $PIN_TOOL -t $DCACHE -- ./runme_small.sh >> $TRACES/$CONSUMER-jpeg-small

cd $MI_BENCH/$NETWORK/dijkstra
sudo $PIN_TOOL -t $DCACHE -- ./runme_large.sh >> $TRACES/$NETWORK-dijkstra-large
sudo $PIN_TOOL -t $DCACHE -- ./runme_small.sh >> $TRACES/$NETWORK-dijkstra-small

cd $MI_BENCH/$NETWORK/patricia
sudo $PIN_TOOL -t $DCACHE -- ./runme_large.sh >> $TRACES/$NETWORK-patricia-large
sudo $PIN_TOOL -t $DCACHE -- ./runme_small.sh >> $TRACES/$NETWORK-patricia-small

cd $MI_BENCH/$OFFICE/stringsearch
sudo $PIN_TOOL -t $DCACHE -- ./runme_large.sh >> $TRACES/$OFFICE-stringsearch-large
sudo $PIN_TOOL -t $DCACHE -- ./runme_small.sh >> $TRACES/$OFFICE-stringsearch-small

cd $MI_BENCH/$SECURITY/blowfish
sudo $PIN_TOOL -t $DCACHE -- ./runme_large.sh >> $TRACES/$SECURITY-blowfish-large
sudo $PIN_TOOL -t $DCACHE -- ./runme_small.sh >> $TRACES/$SECURITY-blowfish-small

cd $MI_BENCH/$SECURITY/sha
sudo $PIN_TOOL -t $DCACHE -- ./runme_large.sh >> $TRACES/$SECURITY-sha-large
sudo $PIN_TOOL -t $DCACHE -- ./runme_small.sh >> $TRACES/$SECURITY-sha-small

cd $MI_BENCH/$TELECOMM/adpcm
sudo $PIN_TOOL -t $DCACHE -- ./runme_large.sh >> $TRACES/$TELECOMM-adpcm-large
sudo $PIN_TOOL -t $DCACHE -- ./runme_small.sh >> $TRACES/$TELECOMM-adpcm-small

cd $MI_BENCH/$TELECOMM/CRC32
sudo $PIN_TOOL -t $DCACHE -- ./runme_large.sh >> $TRACES/$TELECOMM-crc32-large
sudo $PIN_TOOL -t $DCACHE -- ./runme_small.sh >> $TRACES/$TELECOMM-crc32-small

cd $MI_BENCH/$TELECOMM/FFT
sudo $PIN_TOOL -t $DCACHE -- ./runme_large.sh >> $TRACES/$TELECOMM-fft-large
sudo $PIN_TOOL -t $DCACHE -- ./runme_small.sh >> $TRACES/$TELECOMM-fft-small

cd $MI_BENCH/$TELECOMM/gsm
sudo $PIN_TOOL -t $DCACHE -- ./runme_large.sh >> $TRACES/$TELECOMM-gsm-large
sudo $PIN_TOOL -t $DCACHE -- ./runme_small.sh >> $TRACES/$TELECOMM-gsm-small
