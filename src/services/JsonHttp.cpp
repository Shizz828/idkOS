#include "JsonHttp.h"
#include "JsonDecode.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
namespace spark {
extern const uint8_t caBundle[] asm("_binary_data_cert_x509_crt_bundle_bin_start");
// HTTPClient decodes chunked responses while this sink bounds their total size.
class BoundedBody : public Stream {
    size_t limit;
    uint32_t started=millis();
public:
    String body;
    bool overflow=false;
    explicit BoundedBody(size_t max):limit(max) { body.reserve(4096); }
    using Print::write;
    size_t write(uint8_t c) override { return write(&c,1); }
    size_t write(const uint8_t* buffer,size_t count) override {
        if(body.length()+count>limit || uint32_t(millis()-started)>20000) { overflow=true; return 0; }
        return body.concat(reinterpret_cast<const char*>(buffer),count)?count:0;
    }
    int available() override { return 0; }
    int read() override { return -1; }
    int peek() override { return -1; }
    void flush() override {}
};
HttpResult fetchJson(const char* url, DynamicJsonDocument& doc, JsonVariantConst filter, char* error, size_t errorSize, size_t maxBody, const char* authHeader, const char* authKey, size_t maxDocument) {
    HttpResult result;
    WiFiClientSecure client;
    client.setCACertBundle(caBundle);
    client.setHandshakeTimeout(10);
    HTTPClient http;
    http.setConnectTimeout(8000); http.setTimeout(8000);
    http.setReuse(false); http.useHTTP10(true);
    if(!http.begin(client,url)) { snprintf(error,errorSize,"HTTPS setup failed"); result.failure=HttpFailure::Connection; return result; }
    http.addHeader("User-Agent","idkOS/1.1.1 (Cardputer client)");
    http.addHeader("Accept","application/json");
    if(authHeader && authKey) http.addHeader(authHeader,authKey);
    const char* headers[]={"Retry-After"}; http.collectHeaders(headers,1);
    int code=http.GET();
    result.code=code;
    char tlsText[64]{};
    int tlsError=result.tlsError=code<0?client.lastError(tlsText,sizeof(tlsText)):0;
    String retry=http.header("Retry-After");
    if(retry.length() && retry.length()<10) {
        bool numeric=true; for(unsigned i=0;i<retry.length();++i) if(!isDigit(retry[i])) numeric=false;
        if(numeric) result.retryAfterSeconds=uint32_t(retry.toInt());
    }
    bool ok=false;
    HttpFailure failure=HttpFailure::None;
    // Distinguish TLS and connection failures from HTTP status errors;
    // every negative code is a transport failure, never an HTTP status.
    if(code<0 && tlsError) { snprintf(error,errorSize,"TLS error %d",tlsError); failure=HttpFailure::Tls; }
    else if(code<0) { snprintf(error,errorSize,"HTTPS connection failed"); failure=HttpFailure::Connection; }
    else if(code!=200) { snprintf(error,errorSize,"HTTP error %d",code); failure=HttpFailure::Http; }
    else if(http.getSize()>int(maxBody)) { snprintf(error,errorSize,"Response too large"); failure=HttpFailure::Data; }
    else {
        BoundedBody response(maxBody);
        int written=http.writeToStream(&response);
        if(written<0 || response.overflow || response.body.isEmpty()) { snprintf(error,errorSize,"Incomplete response"); failure=HttpFailure::Data; }
        else {
            // Release TLS/HTTP allocations before allocating the JSON document.
            http.end();client.stop();
            auto result=decodeJson(doc,response.body.c_str(),filter,maxDocument);
            if(result==DeserializationError::NoMemory) snprintf(error,errorSize,"JSON memory limit reached");
            else if(result) snprintf(error,errorSize,"Invalid API JSON");
            else ok=true;
            if(!ok) failure=HttpFailure::Data;
        }
    }
    http.end(); result.ok=ok; result.failure=failure; return result;
}
}
