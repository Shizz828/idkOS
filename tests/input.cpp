#include <cassert>
#include <cstring>
#include "../src/core/Input.h"
#include "../src/core/AppRegistry.h"
int main(){
 spark::Input i; auto& k=M5Cardputer.Keyboard;
 k.s.fn=true; k.keys={{11,2}};
 assert(i.poll(0).up); assert(!i.poll(399).up); assert(i.poll(400).up);
 k.keys={{11,3}}; auto e=i.poll(401); assert(e.down && !e.up);
 k.keys={{0,0}}; assert(i.poll(402).sleep); assert(!i.poll(403).sleep);
 k.keys={{13,0}};assert(!i.poll(403).back);
 k.s.fn=false; k.keys={{13,0}}; e=i.poll(404); assert(e.erase && !e.text[0]);
 k.keys={{13,2}}; e=i.poll(405); assert(e.enter && !e.text[0]); assert(!i.poll(406).enter);
 k.keys={{2,2}}; assert(i.poll(407).text[0]=='a'); assert(!i.poll(408).text[0]);
 k.keys={{3,2}}; assert(i.poll(409).text[0]=='b');
 k.keys.clear(); i.poll(410); k.keys={{3,2}}; assert(i.poll(411).text[0]=='b');
 k.keys={{0,1}}; e=i.poll(412);assert(e.tab && !e.text[0]);assert(!i.poll(412).tab); // Tab is not '+'.
 k.s.shift=true; k.keys={{1,2},{2,2}}; e=i.poll(413);
 assert(e.shift && e.text[0]=='A' && e.text[1]==0);
 k.s.shift=false; k.keys.clear(); i.poll(414);
 k.keys={{2,2}}; e=i.poll(415); assert(!e.shift && e.text[0]=='a');
 k.s.fn=true;
 for(int key=1;key<=6;++key) {
  k.keys={{key,0}}; e=i.poll(500+key); assert(e.shortcut=='0'+key && !e.text[0]);
  assert(!i.poll(600+key).shortcut);
 }
 k.keys={{3,2}}; assert(i.poll(700).shortcut=='s'); assert(!i.poll(701).shortcut);
 k.s.fn=false; k.keys={{4,0}}; assert(!i.poll(702).shortcut);
 k.s.fn=true; assert(i.poll(703).shortcut=='4'); // Fn pressed second.
 assert(spark::shortcutApp('1')==spark::CaptureIndex && spark::shortcutApp('2')==spark::WeatherIndex);
 assert(spark::shortcutApp('3')==spark::CryptoIndex && spark::shortcutApp('4')==spark::ConsoleIndex);
 assert(spark::shortcutApp('5')==spark::SettingsIndex && spark::shortcutApp('s')==spark::SshIndex);
 assert(spark::shortcutApp('6')==spark::CalendarIndex);   // Fn+6 Calendar (1.2.0).
 assert(spark::shortcutApp('7')==-1);
 k.s.fn=false; k.s.ctrl=true; k.keys={{3,2}};
 assert(i.poll(704).save); assert(!i.poll(705).save);
 k.s.ctrl=false; assert(!i.poll(706).save);
 k.s.ctrl=true; assert(i.poll(707).save);
 k.keys={{4,1}}; assert(i.poll(708).reminder); assert(!i.poll(709).reminder);
 k.s.ctrl=false; assert(!i.poll(710).reminder);
 k.s.ctrl=true; assert(i.poll(711).reminder);
 // SSH receives real control bytes, without leaking them into text editors.
 k.s.ctrl=true;k.s.fn=false;k.keys.clear();i.poll(790);k.keys={{2,2}};
 e=i.poll(791);assert(e.control==1 && !e.text[0]);assert(!i.poll(792).control);
 // Fn+L is a single idle edge in either chord order; L alone is text.
 k.s.ctrl=false;k.s.fn=false;k.keys.clear();i.poll(780);
 k.keys={{10,2}};e=i.poll(781);assert(!e.idle && e.text[0]);
 k.s.fn=true;e=i.poll(782);assert(e.idle && !e.text[0] && !e.sleep && !e.shortcut);assert(!i.poll(783).idle);
 k.keys.clear();i.poll(784);k.keys={{10,2}};assert(i.poll(785).idle);
 k.keys={{0,0}};e=i.poll(786);assert(e.sleep && !e.idle);
 // Esc is navigation, never printable text; Fn+Esc exclusively sleeps.
 k.s.ctrl=false;k.s.fn=false;k.keys.clear();i.poll(800);
 k.keys={{0,0}};e=i.poll(801);assert(e.back && !e.sleep && !e.text[0] && e.keyPressed);
 e=i.poll(802);assert(!e.back && !e.sleep && !e.text[0] && !e.keyPressed);
 k.keys.clear();i.poll(803);k.keys={{0,0}};assert(i.poll(804).back);
 k.keys.clear();k.s.fn=true;i.poll(805);k.keys={{0,0}};
 e=i.poll(806);assert(e.sleep && !e.back && !e.text[0]);
 e=i.poll(807);assert(!e.sleep && !e.back);
 k.s.fn=false;e=i.poll(808);assert(!e.back && !e.sleep && !e.text[0]);
 k.keys.clear();i.poll(809);k.keys={{0,0}};assert(i.poll(810).back);
 k.s.fn=true;e=i.poll(811);assert(e.sleep && !e.back); // Either chord order.
 // Fresh key edges remain available for consuming the wake key in the loop.
 k.keys.clear();k.s.fn=false;i.poll(812);k.keys={{2,2}};
 e=i.poll(813);assert(e.keyPressed && e.text[0]=='a');assert(!i.poll(814).keyPressed);
 k.keys.clear();i.poll(815);k.s.shift=true;k.keys={{0,0}};
 e=i.poll(816,true);assert(!e.back && e.text[0]=='~'); // SSH can type ~/paths.
 k.keys.clear();i.poll(817);k.keys={{0,0}};
 e=i.poll(818);assert(e.back && !e.text[0]); // Other app input stays unchanged.
 // Terminal modifier resolution is independent of the library's getKey result.
 {
  spark::Input terminal;uint32_t t=1000;
  k.s={};k.s.shift=true;k.keys={{0,0}};
  e=terminal.poll(t,true);assert(!e.back && !e.sleep && !strcmp(e.text,"~"));
  assert(!terminal.poll(t+1,true).text[0]);
  k.s.shift=false;e=terminal.poll(t+2,true);assert(!e.back && !e.text[0]);
  k.keys.clear();terminal.poll(t+3,true);
  // Esc can arrive one FIFO event before Shift; no premature Back is emitted.
  k.keys={{0,0}};e=terminal.poll(t+10,true);assert(!e.back);
  k.s.shift=true;e=terminal.poll(t+18,true);assert(!e.back && !strcmp(e.text,"~"));
  k.keys.clear();k.s={};terminal.poll(t+19,true);
  // Plain Esc still produces exactly one Back, on release or after the window.
  k.keys={{0,0}};assert(!terminal.poll(t+30,true).back);
  k.keys.clear();assert(terminal.poll(t+31,true).back);
  assert(!terminal.poll(t+32,true).back);
  k.keys={{0,0}};assert(!terminal.poll(t+40,true).back);
  assert(terminal.poll(t+75,true).back);assert(!terminal.poll(t+76,true).back);
  k.keys.clear();terminal.poll(t+77,true);
  // Fn takes priority even with Shift held. Releasing Fn cannot trigger Back.
  k.s.fn=k.s.shift=true;k.keys={{0,0}};e=terminal.poll(t+80,true);
  assert(e.sleep && !e.back && !e.text[0]);
  k.s.fn=false;e=terminal.poll(t+81,true);assert(!e.back && !e.text[0]);
  k.keys.clear();terminal.poll(t+82,true);
  k.s.fn=true;k.keys={{11,2}};e=terminal.poll(t+90,true);
  assert(e.up && e.shift && e.fn && !e.back); // SSH Shift+Fn scroll remains intact.
 }
 // Text fields (notes, Wi-Fi password) and menus: Esc=Back, Fn+Esc=Sleep, Shift+Esc='~' only in text.
 for(bool text:{true,false}){
  spark::Input field;uint32_t t=2000;bool menu=!text;
  k.s={};k.keys={{0,0}};e=field.poll(t,text,menu);assert(!e.sleep && !e.text[0] && e.back==!text);
  k.keys.clear();e=field.poll(t+1,text,menu);assert(e.back==text && !e.sleep && !e.text[0]);
  assert(!field.poll(t+2,text,menu).back);
  k.s.fn=true;k.keys={{0,0}};e=field.poll(t+10,text,menu);assert(e.sleep && !e.back && !e.text[0]);
  k.keys.clear();k.s={};e=field.poll(t+11,text,menu);assert(!e.back && !e.sleep);
  k.s.shift=true;k.keys={{0,0}};e=field.poll(t+20,text,menu);
  if(text)assert(!e.back && !e.sleep && !strcmp(e.text,"~"));
  else assert(e.back && !e.sleep && !e.text[0]); // Menus: Shift+Esc stays Back.
  assert(!field.poll(t+21,text,menu).text[0]);
  k.keys.clear();k.s={};e=field.poll(t+22,text,menu);assert(!e.back && !e.text[0]);
 }
 // Menus and lists: plain arrows navigate; Fn+arrow types the printed character.
 {
  spark::Input menu;uint32_t t=2000;k.s={};k.keys.clear();menu.poll(t,false,true);
  const int pos[4][2]={{11,2},{11,3},{10,3},{12,3}};const char printed[]=";.,/";
  for(int a=0;a<4;++a){
   k.s={};k.keys={{pos[a][0],pos[a][1]}};e=menu.poll(t+=10,false,true);
   assert(!e.text[0] && e.up==(a==0) && e.down==(a==1) && e.left==(a==2) && e.right==(a==3));
   assert(!menu.poll(t+=10,false,true).text[0]); // Held: repeat only, never text.
   k.keys.clear();menu.poll(t+=10,false,true);
   k.s.fn=true;k.keys={{pos[a][0],pos[a][1]}};e=menu.poll(t+=10,false,true);
   assert(e.text[0]==printed[a] && !e.text[1] && !e.up && !e.down && !e.left && !e.right);
   assert(!menu.poll(t+=10,false,true).text[0]);
   k.keys.clear();menu.poll(t+=10,false,true);
  }
  // Fn shortcuts, Fn+L and Fn+Esc are the same in menus.
  k.s={};k.s.fn=true;
  for(int key=1;key<=6;++key){k.keys={{key,0}};assert(menu.poll(t+=10,false,true).shortcut=='0'+key);k.keys.clear();menu.poll(t+=10,false,true);}
  k.keys={{3,2}};assert(menu.poll(t+=10,false,true).shortcut=='s');k.keys.clear();menu.poll(t+=10,false,true);
  k.keys={{10,2}};assert(menu.poll(t+=10,false,true).idle);k.keys.clear();menu.poll(t+=10,false,true);
  k.keys={{0,0}};e=menu.poll(t+=10,false,true);assert(e.sleep && !e.back);k.keys.clear();menu.poll(t+=10,false,true);
  k.s={};k.keys={{0,0}};assert(menu.poll(t+=10,false,true).back);k.keys.clear();menu.poll(t+=10,false,true);
  // Text entry, Terminal and SSH (menuMode off) keep the printed characters and Fn arrows.
  k.keys={{11,2}};e=menu.poll(t+=10);assert(e.text[0]==';' && !e.up);k.keys.clear();menu.poll(t+=10);
  k.keys={{12,3}};e=menu.poll(t+=10,true);assert(e.text[0]=='/' && !e.right);k.keys.clear();menu.poll(t+=10,true);
  k.s.fn=true;k.keys={{10,3}};e=menu.poll(t+=10);assert(e.left && !e.text[0]);
  k.keys.clear();k.s={};
 }
}
