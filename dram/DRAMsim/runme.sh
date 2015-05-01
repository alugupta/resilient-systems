###############################################################################
# RANDOMIZE INPUT With ACCESS DISTRIBUTION
###############################################################################

########################
# DDR3
########################
#./DRAMsim -dram:spd_input mem_system_def/DDR3-1333-6-6-6.spd -dram:refresh 64 -dram:refresh_issue_policy priority -dram:address_mapping_policy sdram_hiperf_map -dram:power_input powerConfig.spd -max:inst 25000000 -cpu:frequency 4000 -stat:dram:cas_per_ras -stat:power drampowerstats -accessdist 0.05 0.35 0.95 0.90

########################
# DDR2
########################
#./DRAMsim -dram:spd_input mem_system_def/DDR2-667-4-4-4.spd -dram:refresh 64 -dram:refresh_issue_policy priority -dram:address_mapping_policy sdram_hiperf_map -dram:power_input powerConfig.spd -max:inst 25000000 -cpu:frequency 4000 -stat:dram:cas_per_ras -stat:power drampowerstats -accessdist 0.05 0.35 0.95 0.90

###############################################################################
# TRACE FILE
###############################################################################

########################
# DDR3
########################
#./DRAMsim -dram:spd_input mem_system_def/DDR3-1333-6-6-6.spd -dram:refresh 64 -dram:refresh_issue_policy priority -dram:address_mapping_policy sdram_hiperf_map -dram:power_input powerConfig.spd -max:inst 25000000 -cpu:frequency 4000 -stat:dram:cas_per_ras -stat:power drampowerstats -trace_file ../gcc.trc

########################
# DDR2
########################
#./DRAMsim -dram:spd_input mem_system_def/DDR2-667-4-4-4.spd -dram:refresh 64 -dram:refresh_issue_policy priority -dram:address_mapping_policy sdram_hiperf_map -dram:power_input mem_system_def/DDR2-667-4-4-4.dsh -max:inst 25000000 -cpu:frequency 4000 -stat:dram:cas_per_ras -stat:power drampowerstats -accessdist 0.05 0.35 0.95 0.90
#./DRAMsim -dram:spd_input mem_system_def/DDR2-667-4-4-4.spd -dram:refresh 64 -dram:refresh_issue_policy priority -dram:address_mapping_policy sdram_hiperf_map -dram:power_input mem_system_def/DDR2-667-4-4-4.dsh -max:inst 25000000 -cpu:frequency 4000 -stat:dram:cas_per_ras -stat:power drampowerstats -trace_file ../gcc.trc

./DRAMsim -dram:type ddr2 -dram:spd_input mem_system_def/DDR2-667-4-4-4.spd -dram:refresh 64 -dram:refresh_issue_policy priority -dram:address_mapping_policy sdram_hiperf_map -dram:power_input mem_system_def/DDR2-667-4-4-4.dsh -max:inst 25000000 -cpu:frequency 4000 -stat:dram:cas_per_ras -stat:power drampowerstats -trace_file ../gcc.trc
