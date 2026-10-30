#pragma once
#include "SshData.h"
namespace spark {
// Small ASCII/VT100 screen for an interactive shell, not a full-screen editor.
class SshTerminal {
    char cells[SshRows][SshColumns+1]{};
    char history[48][SshColumns+1]{};
    unsigned x=0,y=0,savedX=0,savedY=0,historyHead=0,historyCount=0,offset=0;
    enum Parse {Normal,Escape,Csi,Osc,OscEscape,String,StringEscape} parse=Normal;
    unsigned params[4]{},param=0;
    bool wrap=false;
    void blank(unsigned row){memset(cells[row],' ',SshColumns);cells[row][SshColumns]=0;}
    void lineFeed(){wrap=false;if(y+1<SshRows){++y;return;}memcpy(history[historyHead],cells[0],sizeof(cells[0]));historyHead=(historyHead+1)%48;if(historyCount<48)++historyCount;if(offset && offset<historyCount)++offset;memmove(cells,cells+1,sizeof(cells)-sizeof(cells[0]));blank(SshRows-1);}
    unsigned value(unsigned index,unsigned fallback=1) const {return params[index]?params[index]:fallback;}
    void csi(char c){
        unsigned n=value(0);
        if(c=='A')y=n>y?0:y-n;
        else if(c=='B')y=y+n>=SshRows?SshRows-1:y+n;
        else if(c=='C')x=x+n>=SshColumns?SshColumns-1:x+n;
        else if(c=='D')x=n>x?0:x-n;
        else if(c=='G')x=n>SshColumns?SshColumns-1:n-1;
        else if(c=='H'||c=='f'){y=n>SshRows?SshRows-1:n-1;n=value(1);x=n>SshColumns?SshColumns-1:n-1;}
        else if(c=='K'){unsigned from=params[0]==1?0:x,to=params[0]==0?SshColumns:x+1;if(params[0]==2){from=0;to=SshColumns;}if(params[0]<=2)memset(cells[y]+from,' ',to-from);}
        else if(c=='J'){
            if(params[0]==2 || params[0]==3){for(unsigned i=0;i<SshRows;++i)blank(i);}
            else if(params[0]==0){memset(cells[y]+x,' ',SshColumns-x);for(unsigned i=y+1;i<SshRows;++i)blank(i);}
            else if(params[0]==1){for(unsigned i=0;i<y;++i)blank(i);memset(cells[y],' ',x+1);}
        } else if(c=='s'){savedX=x;savedY=y;}else if(c=='u'){x=savedX;y=savedY;}
        // SGR, modes and unsupported CSI sequences have no visible payload.
        wrap=false;
    }
public:
    SshTerminal(){for(unsigned i=0;i<SshRows;++i)blank(i);}
    void feed(const char* data,size_t size){for(size_t i=0;i<size;++i){unsigned char c=data[i];
        if(parse==Osc || parse==String){if(c==27)parse=parse==Osc?OscEscape:StringEscape;else if(c==7 && parse==Osc)parse=Normal;continue;}
        if(parse==OscEscape || parse==StringEscape){if(c=='\\')parse=Normal;else parse=parse==OscEscape?Osc:String;continue;}
        if(parse==Escape){parse=Normal;if(c=='['){parse=Csi;memset(params,0,sizeof(params));param=0;}else if(c==']')parse=Osc;else if(c=='P'||c=='^'||c=='_')parse=String;else if(c=='7'){savedX=x;savedY=y;}else if(c=='8'){x=savedX;y=savedY;}continue;}
        if(parse==Csi){if(c>='0'&&c<='9'){if(params[param]<999)params[param]=params[param]*10+c-'0';}else if(c==';'){if(param<3)++param;}else if(c>=0x40&&c<=0x7e){csi(char(c));parse=Normal;}else if(c==27)parse=Escape;continue;}
        if(c==27){parse=Escape;continue;}if(c=='\r'){x=0;wrap=false;continue;}if(c=='\n'){lineFeed();continue;}if(c==8 || c==127){if(x)--x;wrap=false;continue;}
        if(c=='\t'){unsigned n=8-x%8;while(n--)feed(" ",1);continue;}
        if(c<32 || c>126)continue;
        if(wrap){x=0;lineFeed();}cells[y][x]=char(c);if(x+1==SshColumns)wrap=true;else ++x;
    }}
    void scroll(int delta){int next=int(offset)+delta;offset=next<0?0:unsigned(next)>historyCount?historyCount:unsigned(next);}
    void snapshot(SshSnapshot& out)const{
        for(unsigned row=0;row<SshRows;++row){int index=int(historyCount)+int(row)-int(offset);if(index<int(historyCount))memcpy(out.rows[row],history[(historyHead+48-historyCount+unsigned(index))%48],sizeof(cells[0]));else memcpy(out.rows[row],cells[index-int(historyCount)],sizeof(cells[0]));}
        out.cursorX=x;out.cursorY=y;out.scroll=offset;
    }
};
}
