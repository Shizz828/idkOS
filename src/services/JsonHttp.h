#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
namespace spark {
// Failure class, so callers can tell transport, TLS, HTTP status and body faults apart.
enum class HttpFailure : uint8_t { None, Connection, Tls, Http, Data };
struct HttpResult { int code=0; uint32_t retryAfterSeconds=0; bool ok=false; HttpFailure failure=HttpFailure::None; int tlsError=0; };
// Called only by the shared network worker; never by drawing/input handlers.
HttpResult fetchJson(const char* url, DynamicJsonDocument& doc, JsonVariantConst filter,
                     char* error, size_t errorSize, size_t maxBody=49152,
                     const char* authHeader=nullptr, const char* authKey=nullptr, size_t maxDocument=0);
}
