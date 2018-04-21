
##Compile:

* `make`:			complie all files into executable file. 
* `make clean`:	clean all compiled files

##Usage:
* `./opt -v ./TestCodes/algred/algred.i`: 		optimize with Local Value Numbering
* `./opt -u ./TestCodes/algred/algred.i`: 	optimize with Loop Unrolling
* `./opt -v -u ./TestCodes/algred/algred.i`: 			optimize with Local Value Numbering and Loop Unrolling
* `./opt -u -v ./TestCodes/algred/algred.i`: 			optimize with Loop Unrolling and Local Value Numbering
*  All the output will be written in output.i
*  `./sim ./output.i`: 		test with Sim, an ILOC simulator to measure
the number of cycles

##Test:
1.	The optimizer under 4 modes has been tested under all testcases in folder `./TestCodes` and pass them all. All the output results are correct and the cycles are decreased under `-u` `-v` `-u -v` `-v -u` these 4 modes.
2.	Take `algred` for example. Entering number of 20 and the output is 168020. The number of cycles under different 4 modes are:
	* `-v` `Executed 99368 instructions and 99368 operations in 147368 cycles.`
	* `-u` `Executed 86568 instructions and 86568 operations in 150568 cycles.`
	* `-v -u` `Executed 86568 instructions and 86568 operations in 134568 cycles.`
	* `-u -v` `Executed 86568 instructions and 86568 operations in 105768 cycles.`