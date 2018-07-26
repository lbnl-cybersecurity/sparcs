/*
 * Copyright (C) 2016 Chuck McParland All rights reserved.
 */

/* 
 * File:   newsimpletest.cpp
 * Author: mcp
 *
 * Created on January 25, 2016, 5:07 AM
 */

#include <stdlib.h>
#include <iostream>
#include "dataFileInstance.h"

/*
 * Simple C++ Test Suite
 */

//namespace serviceDataDiscovery {
using namespace serviceDataDiscovery;

void test1() {
    std::cout << "newsimpletest test 1" << std::endl;
    dataFileInstance df;
    df.setDawnTimeHoriz(2015, 12, 1, 0);
    df.setSunsetTimeHoriz(2016, 1, 28, 23);
    int day;
    for(day = 0; day < 50; day++) {
        df.incTimeSpec();
        //std::cout<<"day: "<<day<<" month: "<<mon<<" date: "<<date<<std::endl;
        //printf("date: %d is month: %d, day: %d\n", day, mon, date);
    }
    
}

void test2() {
    std::cout << "newsimpletest test 2" << std::endl;
    std::cout << "%TEST_FAILED% time=0 testname=test2 (newsimpletest) message=error message sample" << std::endl;
}

int main(int argc, char** argv) {
    std::cout << "%SUITE_STARTING% newsimpletest" << std::endl;
    std::cout << "%SUITE_STARTED%" << std::endl;

    std::cout << "%TEST_STARTED% test1 (newsimpletest)" << std::endl;
    test1();
    std::cout << "%TEST_FINISHED% time=0 test1 (newsimpletest)" << std::endl;

    std::cout << "%TEST_STARTED% test2 (newsimpletest)\n" << std::endl;
    //test2();
    std::cout << "%TEST_FINISHED% time=0 test2 (newsimpletest)" << std::endl;

    std::cout << "%SUITE_FINISHED% time=0" << std::endl;

    return (EXIT_SUCCESS);
}

