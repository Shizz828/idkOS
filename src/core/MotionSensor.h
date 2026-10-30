#pragma once
#include <M5Cardputer.h>
#include "MotionWake.h"
namespace spark {
// BMI270 accelerometer, polled by timer while the idle board or automatic
// screen-off is dark. The stock ADV leaves INT1/INT2 unconnected, so no IMU
// interrupt wake is used.
class MotionSensor {
    MotionWake filter;
    m5::IMU_Base* imu=nullptr;
    uint32_t last=0;
    bool monitoring=false,available=false;
    bool write(uint8_t reg,uint8_t value){bool ok=imu->writeRegister8(reg,value);delay(1);return ok;}
    bool read(uint8_t reg,uint8_t& value){bool ok=imu->readRegister(reg,&value,1);delay(1);return ok;}
public:
    bool begin(){
        if(!M5.Imu.isEnabled() || M5.Imu.getType()!=m5::imu_bmi270)return false;
        imu=M5.Imu.getImuInstancePtr(0);if(!imu)return false;
        uint8_t chip=0,internal=0;
        bool ok=read(0x00,chip) && chip==0x24 && read(0x21,internal) && (internal&0x0f)==1;
        if(!ok)return false;
        // Datasheet low-power setup; wait >=450us after writes in suspend/APS.
        ok=write(0x7c,0x00) && ok;
        ok=write(0x7d,0x00) && ok;
        ok=write(0x40,0x17) && ok; // Accel 50Hz / 2 repetitions; gyro off.
        ok=write(0x41,0x00) && ok; // +/-2g, 16384LSB/g.
        ok=write(0x58,0x00) && ok; // No routed IMU interrupt on stock ADV.
        ok=write(0x7c,0x03) && ok;
        uint8_t conf=0,range=0,power=0;
        ok=read(0x40,conf) && conf==0x17 && ok;
        ok=read(0x41,range) && range==0 && ok;
        ok=read(0x7d,power) && power==0 && ok;
        available=ok;return available;
    }
    bool enabled()const{return available;}
    // Accelerometer runs only while motionReady; any other state powers it off.
    bool poll(uint32_t now,bool motionReady){
        if(!available)return false;
        if(!motionReady){
            if(monitoring){available=write(0x7d,0x00);monitoring=false;}
            return false;
        }
        if(!monitoring){
            if(!write(0x7d,0x04)){available=false;return false;}
            filter.reset(now);last=now-MotionSampleMs;monitoring=true;
        }
        if(uint32_t(now-last)<MotionSampleMs)return false;
        last=now;uint8_t frame[15]{};
        // STATUS.drdy_acc is independent of INT_MAP_DATA. One burst covers
        // STATUS, unused AUX data and all six ACC bytes within the APS limit.
        if(!imu->readRegister(0x03,frame,sizeof(frame)) || !(frame[0]&0x80))return false;
        const uint8_t* data=frame+9;float a[3];
        for(unsigned i=0;i<3;++i)a[i]=int16_t(uint16_t(data[2*i])|(uint16_t(data[2*i+1])<<8))/16384.0f;
        return filter.sample(now,a[0],a[1],a[2]);
    }
};
}
