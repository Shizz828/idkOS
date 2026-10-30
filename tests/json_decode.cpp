#include <cassert>
#include <string>
#include "../src/services/JsonDecode.h"
#include "../src/services/WeatherParser.h"
int main(){
 DynamicJsonDocument filter(512),doc(0);
 filter["type"]=true;
 auto p=filter["features"][0]["properties"];
 for(const char* key:{"event","expires","effective","severity","status","messageType"})p[key]=true;
 assert(!filter.overflowed());
 const char* empty="{\"type\":\"FeatureCollection\",\"features\":[]}";
 assert(!spark::decodeJson(doc,empty,filter.as<JsonVariantConst>(),24576));
 assert(doc.capacity()==2048); // A normal no-alert response no longer needs 24 KB.
 spark::AlertData alerts;assert(spark::parseAlerts(doc.as<JsonVariantConst>(),1704067200,alerts));assert(alerts.known&&!alerts.count);
 std::string body="{\"type\":\"FeatureCollection\",\"features\":[";
 for(int i=0;i<60;++i){if(i)body+=",";body+="{\"properties\":{\"event\":\"Flood Warning\",\"severity\":\"Severe\",\"expires\":\"2024-01-01T03:00:00Z\"}}";}
 body+="]}";
 assert(!spark::decodeJson(doc,body.c_str(),filter.as<JsonVariantConst>(),24576));
 assert(doc.capacity()>2048 && doc.capacity()<=24576);
 assert(spark::parseAlerts(doc.as<JsonVariantConst>(),1704067200,alerts));assert(alerts.count==60);
 doc=DynamicJsonDocument(0);
 assert(spark::decodeJson(doc,body.c_str(),filter.as<JsonVariantConst>(),2048)==DeserializationError::NoMemory);
 assert(doc.capacity()==2048);
 auto malformed=spark::decodeJson(doc,"{broken",filter.as<JsonVariantConst>(),24576);
 assert(malformed && malformed!=DeserializationError::NoMemory);
 assert(doc.capacity()==2048); // Syntax errors must not trigger growth.
}
