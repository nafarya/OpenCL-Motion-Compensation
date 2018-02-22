# OpenCL-based Motion Compensation algorithm

Implemented motion compensation algorithm.

Source.cpp - Host code

clfile.cl  - device code

# Algorithm details

Order of block selection - Full search

Block size - 8 (but you can vary the value as you wish)

Block similarity criteria - SSD (Sum of Squared Differences)

# Profiler INFO:

GPU: AMD Radeon HD 6950

ALUbusy: 72,97%

LDSBankConflict: 0,22%

for more details check [profiler info file](CodeXL_Profiler_Feb-22-2018_11-57-37.csv)

