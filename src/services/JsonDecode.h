#pragma once
#include <ArduinoJson.h>
namespace spark {
// Grow only for documents that need it, with a hard ceiling. Release the old
// allocation before growing so it cannot compete with its replacement.
inline DeserializationError decodeJson(DynamicJsonDocument& doc,const char* body,
                                      JsonVariantConst filter,size_t maximum=0) {
    if(maximum && !doc.capacity())doc=DynamicJsonDocument(maximum<2048?maximum:2048);
    for(;;) {
        if(!doc.capacity())return DeserializationError::NoMemory;
        auto result=deserializeJson(doc,body,DeserializationOption::Filter(filter),DeserializationOption::NestingLimit(16));
        if(result!=DeserializationError::NoMemory || !maximum || doc.capacity()>=maximum)return result;
        size_t next=doc.capacity()>maximum/2?maximum:doc.capacity()*2;
        doc=DynamicJsonDocument(0);
        doc=DynamicJsonDocument(next);
    }
}
}
