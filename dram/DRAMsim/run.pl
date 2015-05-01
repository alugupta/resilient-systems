#!/usr/bin/perl

# optional debug flags such as -debug:dram  -debug:biu  -debug:transaction can all be inserted

$MAX_INST_COUNT		= 25000000;
$CPU_FREQ		= "4000";				# 2000 | 3000 | 4000 | 5000

#$SPD_INPUT = " -dram:spd_input mem_system_def/SDRAM-133-3-3-3.spd ";
#$SPD_INPUT = " -dram:spd_input mem_system_def/DDR-400-3-4-4.spd -dram:power_input mem_system_def/DDR-400-3-4-4.dsh";
#$SPD_INPUT = " -dram:spd_input mem_system_def/DDR2-667-4-4-4.spd -dram:power_input mem_system_def/DDR2-667-4-4-4.dsh";
#$SPD_INPUT = " -dram:spd_input mem_system_def/FBD_DDR2-667-4-4-4.spd -dram:power_input mem_system_def/FBD_DDR2-667-4-4-4.dsh";
#$SPD_INPUT = " -dram:spd_input mem_system_def/DDR3-1333-6-6-6.spd ";

$CONFIG_FLAG = " -dram:refresh 64  -dram:refresh_issue_policy priority -dram:address_mapping_policy sdram_hiperf_map -max:inst $MAX_INST_COUNT -cpu:frequency $CPU_FREQ  ";

#$DEBUG_FLAG		= "-debug:wave -debug:bundle -debug:amb -debug:threshold 0 -debug:trans ";
$DEBUG_FLAG = " ";

$STAT_FLAG = "-stat:dram:power drampowerstats.txt";
#-stat:all dramallstats";

#some fun random distribution. the also arrival distribution is gaussian
$INPUT_FLAG	= " -accessdist 0.05 0.35 0.95 0.99 ";
#$INPUT_FLAG = " -trace_file mcf.trace ";

$CMD = "./DRAMsim $CONFIG_FLAG $STAT_FLAG $DEBUG_FLAG $INPUT_FLAG ";
system("echo $CMD");
system("$CMD");

