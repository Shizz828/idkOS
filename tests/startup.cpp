#include <cassert>
#include <cstring>
#include <vector>
#include "../src/core/StartupSequence.h"
struct Speaker {
 struct Tone {float frequency;uint32_t duration;uint8_t volume;};
 std::vector<Tone> tones;
 uint8_t volume=100;unsigned starts=0,stops=0,primes=0;
 bool available=true,primeOk=true,toneOk=true,playing=false;
 bool begin(){++starts;return available;}
 uint8_t getVolume(){return volume;}
 void setVolume(uint8_t value){volume=value;}
 bool playRaw(const int8_t* data,size_t count,uint32_t rate,bool stereo,unsigned repeat,int channel,bool replace){
  assert(volume==0 && count==128 && rate==16000 && !stereo && repeat==32 && channel==0 && replace);
  for(size_t i=0;i<count;++i)assert(data[i]==0);
  ++primes;playing=primeOk;return primeOk;
 }
 unsigned isPlaying(int channel){assert(channel==0);return playing;}
 bool tone(float frequency,uint32_t duration,int channel,bool replace){
  assert(channel==0 && replace);if(!toneOk)return false;
  tones.push_back({frequency,duration,volume});playing=true;return true;
 }
 void stop(int channel){assert(channel==0);++stops;playing=false;}
};
void normalCycle(uint32_t start) {
 spark::StartupSequence intro;Speaker speaker;intro.begin(start,speaker);
 assert(intro.showing() && intro.soundActive() && speaker.primes==1 && speaker.tones.empty() && speaker.volume==0);
 assert(!intro.update(start+299,speaker) && speaker.tones.empty());
 // A slow playback task must consume its prime before the audible note.
 assert(!intro.update(start+300,speaker) && speaker.tones.empty());
 speaker.playing=false;assert(!intro.update(start+301,speaker));
 assert(speaker.tones.size()==1 && speaker.volume==spark::StartupChimeVolume);
 assert(!intro.update(start+501,speaker) && speaker.tones.size()==1);
 speaker.playing=false;assert(!intro.update(start+502,speaker));assert(speaker.tones.size()==2);
 assert(speaker.tones[0].frequency>speaker.tones[1].frequency);
 for(auto tone:speaker.tones)assert(tone.volume==50);
 // Restore volume only after the second note's buffer is consumed.
 assert(!intro.update(start+662,speaker) && speaker.volume==50);
 speaker.playing=false;assert(!intro.update(start+663,speaker));
 assert(speaker.volume==100 && speaker.stops==1 && !intro.soundActive());
 assert(!strcmp(intro.audioStatus(),"complete"));
 assert(!intro.update(start+899,speaker));assert(intro.update(start+900,speaker));
 assert(!intro.showing() && !intro.update(start+2000,speaker) && speaker.tones.size()==2);
}
int main(){
 static_assert(spark::StartupChimeVolume==50,"rounded 5 percent target");
 for(unsigned cycle=0;cycle<100;++cycle)normalCycle(cycle*3000u);
 normalCycle(UINT32_MAX-99);
 Speaker silent;spark::StartupSequence disabled;disabled.begin(0,silent,false);
 assert(!silent.starts && !silent.primes && silent.tones.empty());assert(disabled.update(900,silent));assert(silent.volume==100);
 Speaker failed;failed.available=false;spark::StartupSequence missing;missing.begin(0,failed);
 assert(missing.update(900,failed) && failed.tones.empty() && failed.volume==100);
 Speaker noPrime;noPrime.primeOk=false;spark::StartupSequence unprimed;unprimed.begin(0,noPrime);
 assert(unprimed.update(900,noPrime) && noPrime.volume==100 && noPrime.tones.empty());
 Speaker stalled;spark::StartupSequence stuck;stuck.begin(0,stalled);
 assert(stuck.update(1500,stalled));assert(!strcmp(stuck.audioStatus(),"playback-timeout") && stalled.volume==100);
 Speaker rejected;spark::StartupSequence noTone;noTone.begin(0,rejected);rejected.playing=false;rejected.toneOk=false;
 assert(!noTone.update(300,rejected));assert(noTone.update(900,rejected));assert(rejected.volume==100 && !strcmp(noTone.audioStatus(),"tone-failed"));
 // Slow service initialization still plays both notes in full after warm-up.
 Speaker slow;spark::StartupSequence late;late.begin(0,slow);slow.playing=false;
 assert(!late.update(2000,slow));assert(slow.tones.size()==1 && slow.volume==50);
 slow.playing=false;assert(!late.update(2200,slow));assert(slow.tones.size()==2);
 slow.playing=false;assert(late.update(2360,slow) && slow.volume==100);
 // A stuck second note cannot trap boot either.
 Speaker second;spark::StartupSequence hung;hung.begin(0,second);second.playing=false;
 assert(!hung.update(300,second));second.playing=false;assert(!hung.update(500,second));
 assert(hung.update(2000,second) && second.volume==100);
}
