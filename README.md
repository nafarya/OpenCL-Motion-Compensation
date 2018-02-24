# OpenCL-based Motion Compensation algorithm

Implemented [variable block motion compensation](https://en.wikipedia.org/wiki/Motion_compensation#Variable_block-size_motion_compensation) algorithm.

Sources:
- [Host code](https://github.com/nafarya/OpenCL-Motion-Compensation/blob/master/testproject/Source.cpp)

- [Device code](https://github.com/nafarya/OpenCL-Motion-Compensation/blob/master/testproject/clfile.cl)

# Algorithm details

- **Order of block selection:** Full search

- **Block size:** 8 (but you can vary the value as you wish)

- **Block similarity criteria:** SSD (Sum of Squared Differences)

# Profiler INFO:

- **GPU:** AMD Radeon HD 6950

- **ALUbusy:** 72,97%

- **LDSBankConflict:** 0,22%

For more details check [profiler info file](CodeXL_Profiler_Feb-22-2018_11-57-37.csv)

# Examples

**Frame0015**
<img src="https://github.com/nafarya/OpenCL-Motion-Compensation/blob/master/examples/frame0015.png">

**Frame0016**
<img src="https://github.com/nafarya/OpenCL-Motion-Compensation/blob/master/examples/frame0016.png">

**Motion compensated frame**
<img src="https://github.com/nafarya/OpenCL-Motion-Compensation/blob/master/examples/compensatedFrame.png">

**Differences between the frame0015 and the frame0016**
<img src="https://github.com/nafarya/OpenCL-Motion-Compensation/blob/master/examples/diff.png">
