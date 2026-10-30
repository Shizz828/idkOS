#include <cassert>
#include "../src/core/ReminderTone.h"
struct Speaker {
 int volume=100,tones=0,stops=0;bool available=true;
 bool begin(){return available;}int getVolume(){return volume;}void setVolume(int n){volume=n;}
 void tone(float frequency,int duration,int channel,bool replace){assert(frequency==880 && duration==240 && channel==0 && replace);++tones;}
 void stop(int channel){assert(channel==0);++stops;}
};
int main(){
 spark::ReminderTone tone;Speaker speaker;tone.play(100,speaker);assert(speaker.volume==48 && speaker.tones==1);
 tone.play(110,speaker);assert(speaker.tones==1);tone.update(359,speaker);assert(speaker.volume==48);
 tone.update(360,speaker);assert(speaker.volume==100 && speaker.stops==1);
 tone.play(400,speaker);assert(speaker.tones==2);tone.update(1000,speaker);assert(speaker.volume==100);
 speaker.available=false;tone.play(1100,speaker);assert(speaker.tones==2);
}
