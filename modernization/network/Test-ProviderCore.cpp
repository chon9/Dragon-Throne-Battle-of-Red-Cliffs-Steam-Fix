// Isolated adapter state tests: no Steam login or lobby creation.
#include "DTNet.cpp"
#define CHECK(x) do { if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 1;} } while(0)
struct ProviderTest {
    static int Run(){
        Provider p;p.self=111;p.owner=111;
        PlayerState me;me.wire.id=1;me.wire.owner=111;strcpy_s(me.wire.name,"Host");
        p.Upsert(me);CHECK(p.queue.empty());
        PlayerState other;other.wire.id=2;other.wire.owner=222;strcpy_s(other.wire.name,"Guest");
        other.data={1,2,3,4};other.wire.size=4;
        p.Upsert(other);CHECK(p.queue.empty());
        p.players.erase(2);p.local=1;
        HANDLE event=CreateEventW(0,TRUE,FALSE,0);p.eventHandle=event;
        p.Upsert(other);CHECK(p.queue.size()==1&&WaitForSingleObject(event,0)==WAIT_OBJECT_0);
        DPID from=0,to=0;DWORD bytes=0;
        CHECK(p.Receive(&from,&to,0,nullptr,&bytes)==DPERR_BUFFERTOOSMALL);
        std::vector<uint8_t> buf(bytes);
        CHECK(p.Receive(&from,&to,DPRECEIVE_PEEK,buf.data(),&bytes)==DP_OK&&p.queue.size()==1);
        auto* msg=(DPMSG_CREATEPLAYERORGROUP*)buf.data();
        CHECK(msg->dpId==2&&msg->dwType==DPSYS_CREATEPLAYERORGROUP&&msg->dwCurrentPlayers==2);
        CHECK(!strcmp(msg->dpnName.lpszShortNameA,"Guest")&&msg->dwDataSize==4&&!memcmp(msg->lpData,other.data.data(),4));
        CHECK((uint8_t*)msg->lpData>=buf.data()&&(uint8_t*)msg->lpData+4<=buf.data()+buf.size());
        CHECK(p.Receive(&from,&to,0,buf.data(),&bytes)==DP_OK&&from==0&&to==1);
        CHECK(p.queue.empty()&&WaitForSingleObject(event,0)==WAIT_TIMEOUT);
        p.Upsert(other);CHECK(p.queue.empty());
        strcpy_s(other.wire.name,"Renamed");p.Upsert(other);CHECK(p.queue.size()==1&&p.queue.front().type==DPSYS_SETPLAYERORGROUPNAME);
        p.queue.clear();p.queuedBytes=0;other.data.push_back(5);other.wire.size=5;p.Upsert(other);CHECK(p.queue.size()==1&&p.queue.front().type==DPSYS_SETPLAYERORGROUPDATA);
        p.queue.clear();p.queuedBytes=0;p.Remove(2);CHECK(!p.players.count(2)&&p.queue.size()==1&&p.queue.front().type==DPSYS_DESTROYPLAYERORGROUP);
        p.Lose();CHECK(p.lost&&p.queue.size()==1&&p.queue.front().type==DPSYS_SESSIONLOST);p.Lose();CHECK(p.queue.size()==1);
        p.Close();CloseHandle(event);
        puts("PASS: no duplicate local/initial roster, remote create marshalling/peek/event, idempotent roster, rename/data/remove and session loss");return 0;
    }
};
int main(){return ProviderTest::Run();}
