/*
 * Copyright (C) 2016 Chuck McParland All rights reserved.
 */

/* 
 * File:   serviceList.cpp
 * Author: mcp
 * 
 * Created on January 18, 2016, 8:16 PM
 */

#include <unordered_map>
#include "dbConnection.h"
#include "serviceInstance.h"
#include "serviceList.h"

using namespace std;
using namespace dbUtilities;

namespace serviceCommon {
    

serviceList::serviceList() {
}

serviceList::serviceList(const serviceList& orig) {
}

serviceList::~serviceList() {
}

}