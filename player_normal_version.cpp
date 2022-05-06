#ifdef __local_test__
#include "lib_bot/Bot.h"
#else
#include <bot.h>
#endif

#include "TuMou2022/Game.h"
#include <stdio.h>
#include <stdlib.h>
#include <stack>

/*
以下为选手可能用到的信息
Coordinate类:
int x, y, z 该点对应的坐标

Point类:
int x, y, z 该点对应的坐标
int MineIdx 资源点序号.-1代表该点没有资源点
int BarrierIdx 障碍物序号.-1代表没有障碍物
int PlayerIdx 玩家编号.-1代表没有玩家，0为红方，1为蓝方

Player类:
Coordinate pos 角色位置
int id //角色的id.0为红方,1为蓝方.
int attack_range 攻击范围
int sight_range 视野范围
int move_range 移动范围
int mine_speed 采集能力
int at 攻击力
int hp 生命值
int mines 当前拥有的mine数量

Mine类:
Coordinate pos 资源点位置
int num 该点资源数量
int belong 资源点所有者.-1为蓝方,1为红方
bool available 是否可采集

Map类:
Point Data 三维数组,存储选手视野范围内地图信息.data[i][j][k]表示坐标(i, j, k)对应的点.
std::vector<Mine> mine 角色视野范围内的mine
std::vector<Coordinate> barrier 角色视野范围内的barrier
std::vector<Player> enemy_unit 角色视野范围内的敌方单位
int nowSize 地图的当前大小 地图初始大小为25*25*25.从第75回合开始缩圈，每回合减小一个单位
int mine_num 角色视野范围内的mine数量
int barrier_num 角色视野范围内的barrier数量
int enemy_num 角色视野范围内的敌方单位数量
bool isValid(int x, int y, int z) 判断某点是否在地图内
bool isValid(Coordinate c) 判断某点是否在地图内
int getDistance(Coordinate a, Coordinate b) 计算a, b两点之间的距离
std::vector<Point> getPoints(int x, int y, int z, int dist) //获取地图内距离x, y, z点距离不大于dist的所有点的信息.

Operation类:
int type 角色的操作类型.-1代表不进行操作,0代表移动，1代表攻击.
Coordinate target 移动或攻击目标
int upgrade 是否升级.0代表不升级,1代表升级.
int upgrade_type 升级的类型.0代表移动速度，1代表攻击范围，2代表采集速度，3代表回血，4代表视野范围，5代表攻击力.

其他:
void getInfo(const Player& player, const Map& map) 更新探索到的地图信息.
const int UPGRADE_COST[6] = { 30, 30, 5, 40, 30, 20 } 每种升级类型需要的mines数量
*/

/*以下部分无需修改*/
Point myData[2 * MAP_SIZE - 1][2 * MAP_SIZE - 1][2 * MAP_SIZE - 1]; //可以在此存储角色探索到的地图信息
std::vector<Point> pointsInView; //角色当前视野范围内的点
std::vector<Point> pointsInAttack; //角色当前攻击范围内的点
std::vector<Point> pointsInMove; //角色当前移动范围内的点
void getInfo(const Player& player, const Map& map)
{
    pointsInView.clear();
    pointsInAttack.clear();
    pointsInMove.clear();
    for(int i = 0; i < 2 * MAP_SIZE - 1; i++)
    {
        for(int j = 0; j < 2 * MAP_SIZE - 1; j++)
        {
            for(int k = 0; k < 2 * MAP_SIZE - 1; k++)
            {
                Coordinate tmp = Coordinate(i, j, k);
                if(map.isValid(tmp))
                {
                    if(map.getDistance(tmp, player.pos) <= player.sight_range - 1)
                    {
                        pointsInView.push_back(map.data[i][j][k]);
                        myData[i][j][k] = map.data[i][j][k];
                    }
                    if(map.getDistance(tmp, player.pos) <= player.attack_range)
                    {
                        pointsInAttack.push_back(map.data[i][j][k]);
                    }
                    if(map.getDistance(tmp, player.pos) <= player.move_range)
                    {
                        pointsInMove.push_back(map.data[i][j][k]);
                    }
                }
            }
        }
    }
}
int vis[2 * MAP_SIZE - 1][2* MAP_SIZE - 1][2 * MAP_SIZE - 1];
/*请修改本函数*/
Operation get_operation(const Player& player, const Map& map)
{
    static int danger=0;          
    static int bleed=0;
    static int round=0;
    static int up_num=0;
    static Coordinate enemy_history_pos;
    static int enemy_move_range=0;
    static int enemy_sight_range=0;
    static int enemy_attack_range=0;
    static int enemy_mines=0;
    static int enemy_at=0;
    static int enemy_hp=0;
    static int love_center=0;
    getInfo(player, map);
    Operation op;
    int not_belong[2]={-1,1};      
    int not_is[2]={1,0};         
    int id=player.id;
    int attack_range = player.attack_range; 
    int move_range = player.move_range;
    int sight_range = player.sight_range; 
    int mine_speed = player.mine_speed; 
    int at = player.at; 
    int hp = player.hp; 
    int mines = player.mines; 
    Coordinate pos={player.pos.x,player.pos.y,player.pos.z};  
    Coordinate center={24,24,24};
    int moveSize=pointsInMove.size();
    int mine_size=map.mine.size();  
    int enemy_num=map.enemy_num;  
    int nowSize=map.nowSize;  
    op.type=0;
    round++;       
    int up_size=49;
    int up[49]={0,0,0,0,4,4,4,4,5,4,1,5,1,5,1,5,1,5,1,5,1,5,1,5,1,0,1,0,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1,4,1};
    int safeSize[101]={0};      
    int hurt[101]={0};
    for(int i=68;i<101;i++){
      hurt[i]=10+(i-67)*5;//可能有问题
    }  
    for(int i=1;i<68;i++){
       hurt[i]=10;
    }
    for(int i=1;i<=30;i++){
     safeSize[i]=25-i/5;
    }
    for(int i=31;i<68;i++){
     safeSize[i]=19-(i-30)/2;
    }
    int dis0=map.getDistance(pos,center);//准备工作

    if(sight_range>=dis0+nowSize&&bleed==0){
    int g=0;
     for(int i=0;i<mine_size;i++){
      if(map.mine[i].belong==not_belong[id])continue;
      if(map.mine[i].num<mine_speed)continue;
      int dis2=map.getDistance(map.mine[i].pos,center);
      if(dis2>nowSize)continue;
      g=1;
      break;
    }
    if(g==0)bleed=1;
    }//决定是否进入流血模式
if(bleed==1){//流血模式

int fear=0;
int have_up=0;//先不用
if(enemy_num>0){//发现敌人
    enemy_history_pos=map.enemy_unit[0].pos;   
    enemy_move_range=map.enemy_unit[0].move_range;   
    enemy_attack_range=map.enemy_unit[0].attack_range;          
    enemy_sight_range=map.enemy_unit[0].sight_range;   
    enemy_mines=map.enemy_unit[0].mines;          
    enemy_at=map.enemy_unit[0].at;     
    enemy_hp=map.enemy_unit[0].hp;        
    danger=1;
    int dis1=map.getDistance(pos,enemy_history_pos);
    if(dis1<=attack_range&&dis1>enemy_attack_range){
        op.type=1;
        op.target=enemy_history_pos;
    }
    else if(dis1<=attack_range&&dis1<=enemy_attack_range){
        int me=((hp+(mines/40)*50)-1)/enemy_at+1;
        int you=((enemy_hp+(enemy_mines/40)*50)-1)/at+1;
        if(me>=you){
        op.type=1;
        op.target=enemy_history_pos;
        }
        else if(me<you){
          enemy_num=0;fear=1;
        }
    }
    else if(dis1>attack_range&&dis1<=enemy_attack_range){
        enemy_num=0; fear=1;//粗糙
    }
    else if(dis1>attack_range&&dis1>enemy_attack_range){
        enemy_num=0;fear=0;//粗糙
    }
}
if(enemy_num==0){//未发现敌人
    if(danger==1&&fear==0){//从简
        danger=0;
    }
    if(danger==0&&fear==0){//搜集资源模式
    int *score=new int[mine_size];
    for(int i=0;i<mine_size;i++){
        score[i]=-1;
    }
    int have_ideal=0;
    for(int i=0;i<mine_size;i++){
      if(map.mine[i].belong==not_belong[id])continue;
      if(map.mine[i].num<mine_speed)continue;
      Coordinate tmp={map.mine[i].pos.x,map.mine[i].pos.y,map.mine[i].pos.z};
      int dis1=map.getDistance(tmp,pos);
      int step=(dis1-1)/move_range+1;if(dis1==0)step=0;
      score[i]=100-step*2;
      if(map.mine[i].belong==0){score[i]+=1;}
      have_ideal=1;
    }
    if(have_ideal==1){
     int test=-1;
     int ideal=-1;
     int dis1=0;
     for(int i=0;i<mine_size;i++){
       if(score[i]>test){test=score[i];ideal=i;dis1=map.getDistance(center,map.mine[i].pos);}
       else if(score[i]>-1&&score[i]==test){
           int dis2=map.getDistance(center,map.mine[i].pos);
           if(dis2<dis1){ideal=i;dis1=dis2;}
       }
     }
     delete[] score;  
     //op.target=map.mine[ideal].pos;
     int toward_ideal=-1;
     int toward_test=100;
     for(int i=0;i<moveSize;i++){
        if(pointsInMove[i].BarrierIdx==0)continue;
        if(pointsInMove[i].PlayerIdx==not_is[id])continue;
        Coordinate tmp={pointsInMove[i].x,pointsInMove[i].y,pointsInMove[i].z};
        int dis1=map.getDistance(tmp,map.mine[ideal].pos);
        if(dis1<toward_test){toward_test=dis1;toward_ideal=i;}
     }
     Coordinate tmp2={pointsInMove[toward_ideal].x,pointsInMove[toward_ideal].y,pointsInMove[toward_ideal].z};
     op.target=tmp2;
    }
    else if(have_ideal==0){ delete[] score;  
     int *score2=new int[moveSize];
     for(int i=0;i<moveSize;i++){
         score2[i]=-1;
     }
     for(int i=0;i<moveSize;i++){
        if(pointsInMove[i].BarrierIdx==0)continue;
        if(pointsInMove[i].PlayerIdx!=-1)continue;
        Coordinate tmp={pointsInMove[i].x,pointsInMove[i].y,pointsInMove[i].z};
        int dis1=map.getDistance(tmp,pos);
        int dis2=map.getDistance(tmp,center);
        int step=(dis1-1)/move_range+1;if(dis1==0)step=0;
  
        if(love_center==0)
        score2[i]=100-dis2;
        else{
            score2[i]=100+dis1*10;
             srand(time(NULL));
             int seed = rand() % 10;
            score2[i]+=seed;
        }
     }
     int test=-1;
     int ideal=-1;
     for(int i=0;i<moveSize;i++){
        if(score2[i]>test){test=score2[i];ideal=i;}
     }
     Coordinate tmp2={pointsInMove[ideal].x,pointsInMove[ideal].y,pointsInMove[ideal].z};
     op.type=0;
     op.target=tmp2;
     delete[] score2;
    }
   
   }//搜集资源模式
   else if(danger==1&&fear==1){//run,体现编程技术的时候到了,目前为搜集模式重用
    int *score=new int[mine_size];
    for(int i=0;i<mine_size;i++){
        score[i]=-1;
    }
    int have_ideal=0;
    for(int i=0;i<mine_size;i++){
      if(map.mine[i].belong==not_belong[id])continue;
      if(map.mine[i].num<mine_speed)continue;
      Coordinate tmp={map.mine[i].pos.x,map.mine[i].pos.y,map.mine[i].pos.z};
      int dis3=map.getDistance(tmp,enemy_history_pos);
      if(dis3<=enemy_attack_range)continue;
      int dis1=map.getDistance(tmp,pos);
      int step=(dis1-1)/move_range+1;if(dis1==0)step=0;
      
      score[i]=100-step*2;
      if(map.mine[i].belong==0){score[i]+=1;}
      have_ideal=1;
    }
    if(have_ideal==1){
     int test=-1;
     int ideal=-1;
     int dis1=0;
     for(int i=0;i<mine_size;i++){
       if(score[i]>test){test=score[i];ideal=i;dis1=map.getDistance(center,map.mine[i].pos);}
       else if(score[i]>-1&&score[i]==test){
           int dis2=map.getDistance(center,map.mine[i].pos);
           if(dis2<dis1){ideal=i;dis1=dis2;}
       }
     }
     delete[] score;  
     //op.target=map.mine[ideal].pos;
     int toward_ideal=-1;
     int toward_test=100;
     for(int i=0;i<moveSize;i++){
        if(pointsInMove[i].BarrierIdx==0)continue;
        if(pointsInMove[i].PlayerIdx==not_is[id])continue;
        Coordinate tmp={pointsInMove[i].x,pointsInMove[i].y,pointsInMove[i].z};
        int dis3=map.getDistance(tmp,enemy_history_pos);
        if(dis3<=enemy_attack_range)continue;
        int dis1=map.getDistance(tmp,map.mine[ideal].pos);
        if(dis1<toward_test){toward_test=dis1;toward_ideal=i;}
     }
     Coordinate tmp2={pointsInMove[toward_ideal].x,pointsInMove[toward_ideal].y,pointsInMove[toward_ideal].z};
     op.target=tmp2;
    }
    else if(have_ideal==0){ delete[] score;  
     int *score2=new int[moveSize];
     for(int i=0;i<moveSize;i++){
         score2[i]=-1;
     }
     for(int i=0;i<moveSize;i++){
        if(pointsInMove[i].BarrierIdx==0)continue;
        if(pointsInMove[i].PlayerIdx!=-1)continue;
        Coordinate tmp={pointsInMove[i].x,pointsInMove[i].y,pointsInMove[i].z};
        int dis3=map.getDistance(tmp,enemy_history_pos);
        if(dis3<=enemy_attack_range)continue;
        int dis1=map.getDistance(tmp,pos);
        int dis2=map.getDistance(tmp,center);
        int step=(dis1-1)/move_range+1;if(dis1==0)step=0;
        if(love_center==0)
        score2[i]=100-dis2;
        else{
            score2[i]=100+dis1*10;
             srand(time(NULL));
             int seed = rand() % 10;
            score2[i]+=seed;
        }
     }
     int test=-1;
     int ideal=-1;
     for(int i=0;i<moveSize;i++){
        if(score2[i]>test){test=score2[i];ideal=i;}
     }
     Coordinate tmp2={pointsInMove[ideal].x,pointsInMove[ideal].y,pointsInMove[ideal].z};
     op.type=0;
     op.target=tmp2;
     delete[] score2;
    }
  
   }//run
}
 //upgrade
op.upgrade=0;
if(mine_speed<30){
    op.upgrade=1;
    op.upgrade_type=2;
}
else{
    if(hp<=50+hurt[round]){
        op.upgrade=1;
        op.upgrade_type=3;
    }
    else{
        if(up_num>=up_size){op.upgrade=0;}
        else{
        if(mines>=UPGRADE_COST[up[up_num]]){op.upgrade=1;op.upgrade_type=up[up_num];up_num++;}
        }
    }
 }
}//流血模式

else if(bleed==0){//前期

int fear=0;
int have_up=0;//先不用
if(enemy_num>0){//发现敌人
    enemy_history_pos=map.enemy_unit[0].pos;   
    enemy_move_range=map.enemy_unit[0].move_range;   
    enemy_attack_range=map.enemy_unit[0].attack_range;          
    enemy_sight_range=map.enemy_unit[0].sight_range;   
    enemy_mines=map.enemy_unit[0].mines;          
    enemy_at=map.enemy_unit[0].at;     
    enemy_hp=map.enemy_unit[0].hp;        
    danger=1;
    int dis1=map.getDistance(pos,enemy_history_pos);
    if(dis1<=attack_range&&dis1>enemy_attack_range){
        op.type=1;
        op.target=enemy_history_pos;
    }
    else if(dis1<=attack_range&&dis1<=enemy_attack_range){
        int me=((hp+(mines/40)*50)-1)/enemy_at+1;
        int you=((enemy_hp+(enemy_mines/40)*50)-1)/at+1;
        if(me>=you){
        op.type=1;
        op.target=enemy_history_pos;
        }
        else if(me<you){
          enemy_num=0;fear=1;
        }
    }
    else if(dis1>attack_range&&dis1<=enemy_attack_range){
        enemy_num=0; fear=1;//粗糙
    }
    else if(dis1>attack_range&&dis1>enemy_attack_range){
        enemy_num=0;fear=0;//粗糙
    }
}//发现敌人

if(enemy_num==0){//未发现敌人

    if(danger==1&&fear==0){//从简
        danger=0;
    }
    if(danger==0&&fear==0){//搜集资源模式
    
    int *score=new int[mine_size];
    for(int i=0;i<mine_size;i++){
        score[i]=-1;
    }
    int have_ideal=0;
    for(int i=0;i<mine_size;i++){
      if(map.mine[i].belong==not_belong[id])continue;
      if(map.mine[i].num<mine_speed)continue;
      Coordinate tmp=map.mine[i].pos;
      int dis1=map.getDistance(tmp,pos);
      int dis2=map.getDistance(tmp,center);
      int step=(dis1-1)/move_range+1;if(dis1==0)step=0;
      if(dis2>safeSize[round+step]+1)continue;
      score[i]=100-step*2;
      if(map.mine[i].belong==0){score[i]+=1;}
      have_ideal=1;
    }
    if(have_ideal==1){
     int test=-1;
     int ideal=-1;
     int dis1=0;
     for(int i=0;i<mine_size;i++){
       if(score[i]>test){test=score[i];ideal=i;dis1=map.getDistance(center,map.mine[i].pos);}
       else if(score[i]>-1&&score[i]==test){
           int dis2=map.getDistance(center,map.mine[i].pos);
           if(dis2<dis1){ideal=i;dis1=dis2;}
       }
     }
      delete[] score;  
     //op.target=map.mine[ideal].pos;
     int toward_ideal=-1;
     int toward_test=100;
     for(int i=0;i<moveSize;i++){
        if(pointsInMove[i].BarrierIdx==0)continue;
        if(pointsInMove[i].PlayerIdx==not_is[id])continue;
        Coordinate tmp={pointsInMove[i].x,pointsInMove[i].y,pointsInMove[i].z};
        int dis1=map.getDistance(tmp,map.mine[ideal].pos);
        if(dis1<toward_test){toward_test=dis1;toward_ideal=i;}
     }
     Coordinate tmp2={pointsInMove[toward_ideal].x,pointsInMove[toward_ideal].y,pointsInMove[toward_ideal].z};
     op.target=tmp2;
    }
    else if(have_ideal==0){
        delete[] score;  
     int *score2=new int[moveSize];
     for(int i=0;i<moveSize;i++){
         score2[i]=-1;
     }
     for(int i=0;i<moveSize;i++){
        if(pointsInMove[i].BarrierIdx==0)continue;
        if(pointsInMove[i].PlayerIdx!=-1)continue;
        Coordinate tmp={pointsInMove[i].x,pointsInMove[i].y,pointsInMove[i].z};
        int dis1=map.getDistance(tmp,pos);
        int dis2=map.getDistance(tmp,center);
        int step=(dis1-1)/move_range+1;if(dis1==0)step=0;
        if(dis2>safeSize[round+step]+1)continue;
        if(love_center==0)
        score2[i]=100-dis2;
        else{
            score2[i]=100+dis1*10;
             srand(time(NULL));
             int seed = rand() % 10;
            score2[i]+=seed;
        }
     }
     int test=-1;
     int ideal=-1;
     for(int i=0;i<moveSize;i++){
        if(score2[i]>test){test=score2[i];ideal=i;}
     }
     Coordinate tmp2={pointsInMove[ideal].x,pointsInMove[ideal].y,pointsInMove[ideal].z};
     op.type=0;
     op.target=tmp2;
     delete[] score2;
    }
   }//搜集资源模式
   else if(danger==1&&fear==1){//run,体现编程技术的时候到了,目前为搜集模式重用
   
    int *score=new int[mine_size];
    for(int i=0;i<mine_size;i++){
        score[i]=-1;
    }
    int have_ideal=0;
    for(int i=0;i<mine_size;i++){
      if(map.mine[i].belong==not_belong[id])continue;
      if(map.mine[i].num<mine_speed)continue;
      Coordinate tmp={map.mine[i].pos.x,map.mine[i].pos.y,map.mine[i].pos.z};
      int dis3=map.getDistance(tmp,enemy_history_pos);
      if(dis3<=enemy_attack_range)continue;
      int dis1=map.getDistance(tmp,pos);
      int dis2=map.getDistance(tmp,center);
      int step=(dis1-1)/move_range+1;if(dis1==0)step=0;
      if(dis2>safeSize[round+step]+1)continue;
      score[i]=100-step*2;
      if(map.mine[i].belong==0){score[i]+=1;}
      have_ideal=1;
    }
    if(have_ideal==1){
     int test=-1;
     int ideal=-1;
     int dis1=0;
     for(int i=0;i<mine_size;i++){
       if(score[i]>test){test=score[i];ideal=i;dis1=map.getDistance(center,map.mine[i].pos);}
       else if(score[i]>-1&&score[i]==test){
           int dis2=map.getDistance(center,map.mine[i].pos);
           if(dis2<dis1){ideal=i;dis1=dis2;}
       }
     }
     delete[] score;  
     //op.target=map.mine[ideal].pos;
     int toward_ideal=-1;
     int toward_test=100;
     for(int i=0;i<moveSize;i++){
        if(pointsInMove[i].BarrierIdx==0)continue;
        if(pointsInMove[i].PlayerIdx==not_is[id])continue;
        Coordinate tmp={pointsInMove[i].x,pointsInMove[i].y,pointsInMove[i].z};
        int dis3=map.getDistance(tmp,enemy_history_pos);
        if(dis3<=enemy_attack_range)continue;
        int dis1=map.getDistance(tmp,map.mine[ideal].pos);
        if(dis1<toward_test){toward_test=dis1;toward_ideal=i;}
     }
     Coordinate tmp2={pointsInMove[toward_ideal].x,pointsInMove[toward_ideal].y,pointsInMove[toward_ideal].z};
     op.target=tmp2;
    }
    else if(have_ideal==0){
     delete[] score;  
     int *score2=new int[moveSize];
     for(int i=0;i<moveSize;i++){
         score2[i]=-1;
     }
     for(int i=0;i<moveSize;i++){
        if(pointsInMove[i].BarrierIdx==0)continue;
        if(pointsInMove[i].PlayerIdx==not_is[id])continue;
        Coordinate tmp={pointsInMove[i].x,pointsInMove[i].y,pointsInMove[i].z};
        int dis3=map.getDistance(tmp,enemy_history_pos);
        if(dis3<=enemy_attack_range)continue;
        int dis1=map.getDistance(tmp,pos);
        int dis2=map.getDistance(tmp,center);
        int step=(dis1-1)/move_range+1;if(dis1==0)step=0;
        if(dis2>safeSize[round+step]+1)continue;
        if(love_center==0)
        score2[i]=100-dis2;
        else{
            score2[i]=100+dis1*10;
            srand(time(NULL));
            int seed = rand() % 10;
            score2[i]+=seed;
        }
     }
     int test=-1;
     int ideal=-1;
     for(int i=0;i<moveSize;i++){
        if(score2[i]>test){test=score2[i];ideal=i;}
     }
     Coordinate tmp2={pointsInMove[ideal].x,pointsInMove[ideal].y,pointsInMove[ideal].z};
     op.type=0;
     op.target=tmp2;
     delete[] score2;
    }
   }//run
   
}//未发现敌人

 //upgrade
op.upgrade=0;
if(mine_speed<30){
    op.upgrade=1;
    op.upgrade_type=2;
}
else{
    if(hp<=50){
        op.upgrade=1;
        op.upgrade_type=3;
    }
    else{
        if(up_num>=up_size){op.upgrade=0;}
        else{
        if(mines>=UPGRADE_COST[up[up_num]]){op.upgrade=1;op.upgrade_type=up[up_num];up_num++;}
        }
    }
 }
 
}//前期
   if(op.type==0&&map.getDistance(pos,op.target)==0){op.type=1;op.target=enemy_history_pos;}
   if(op.type==0&&map.getDistance(center,op.target)<4){love_center=1;}
   return op;
}

/*以下部分无需修改*/
Game *game;

int main()
{
    char *s = bot_recv();
    int side;
    int seed;
    sscanf(s, "%d%d", &side, &seed);
    free(s);
    game = new Game(side, seed);

    game->proc();

    return 0;
}