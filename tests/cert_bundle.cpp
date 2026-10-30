#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
// The HTTPS trust bundle must stay well-formed and sorted (the ESP32 verifier
// binary-searches it), and must cover the roots our services chain to.
int main(){
    FILE* f=fopen("data/cert/x509_crt_bundle.bin","rb");assert(f);
    std::vector<unsigned char> d;int c;while((c=fgetc(f))!=EOF)d.push_back((unsigned char)c);fclose(f);
    assert(d.size()>2);size_t count=(d[0]<<8)|d[1],i=2;
    std::vector<std::vector<unsigned char>> names;
    for(size_t n=0;n<count;++n){
        assert(i+4<=d.size());size_t name=(d[i]<<8)|d[i+1],key=(d[i+2]<<8)|d[i+3];i+=4;
        assert(name && key && i+name+key<=d.size());
        names.emplace_back(d.begin()+i,d.begin()+i+name);i+=name+key;
    }
    assert(i==d.size());
    for(size_t n=1;n<names.size();++n)assert(names[n-1]<names[n]);
    // Each root's subject ends with its CN, so match the exact CN at the end plus the organization.
    auto has=[&](const std::string& cn,const char* org){
        for(const auto& s:names){std::string text(s.begin(),s.end());
            if(text.size()>=cn.size() && !text.compare(text.size()-cn.size(),cn.size(),cn) && text.find(org)!=std::string::npos)return true;}
        return false;};
    // CoinGecko: WE1 <- GTS Root R4, served cross-signed by GlobalSign Root CA (R1).
    assert(has("GlobalSign Root CA","GlobalSign nv-sa"));assert(has("GTS Root R4","Google Trust Services LLC"));
    assert(has("GTS Root R1","Google Trust Services LLC"));
    // Open-Meteo and the weather/location services: Let's Encrypt (ISRG).
    assert(has("ISRG Root X1","Internet Security Research Group"));
}
