//
// Created by Administrator on 2020/4/3.
//

#ifndef GENERIC_ALGORITHM_TYPE_H
#define GENERIC_ALGORITHM_TYPE_H

#include <string>
#include "include/jsonxx.h"

struct SerializableJSONObject
{
    virtual std::string serialize(){};
    virtual jsonxx::Object object(){};
    virtual void unserialize(const std::string &){};

};

#endif //GENERIC_ALGORITHM_TYPE_H
