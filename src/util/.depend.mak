myGetChar.o: myGetChar.cpp
myString.o: myString.cpp
test_random_generator_intel.o: test_random_generator_intel.cpp \
 ../../include/intel_rand.h
util.o: util.cpp rnGen.h myUsage.h
