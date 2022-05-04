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
    static int danger=0;           //准备工作
    static int round=0;
    static int up_num=0;
    static Coordinate enemy_history_pos;
    static int enemy_move_range=0;
    static int enemy_sight_range=0;
    static int enemy_attack_range=0;
    getInfo(player, map);
    Operation op;
    int not_belong[2]={-1,1};               
    int id=player.id;    
    int x = player.pos.x;
    int y = player.pos.y;
    int z = player.pos.z; 
    int attack_range = player.attack_range; 
    int move_range = player.move_range;
    int sight_range = player.sight_range; 
    int mine_speed = player.mine_speed; 
    int at = player.at; 
    int hp = player.hp; 
    int mines = player.mines; 
    Coordinate pos=player.pos;  
    Coordinate center={24,24,24};
    int moveSize=pointsInMove.size();
    int mine_size=map.mine.size();  
    int enemy_num=map.enemy_num;  
    int nowSize=map.nowSize;  
    round++;       
    int safeSize[101]={0};        
    for(int i=1;i<=30;i++){
     safeSize[i]=25-i/5;
    }
    for(int i=31;i<68;i++){
     safeSize[i]=19-(i-30)/2;
    }
op.type=0;
if(danger==0){//寻找资源，have_ideal用于判断有无合适资源点，ideal表示何时资源点编号，score数组用于给资源点打分
    int ideal=0;                             
    int have_ideal=0;
    int score[mine_size]={0};
    for(int i=0;i<mine_size;i++){               
      if(map.mine[i].belong==not_belong[id])continue;
      if(map.mine[i].num<=20)continue;      //太少不要
      int dis_tmp2=map.getDistance(pos,map.mine[i].pos);
      int dis_tmp=map.getDistance(center,map.mine[i].pos);
      int step=(dis_tmp2-1)/move_range+1;if(dis_tmp2==0)step=0;
      if(dis_tmp>safeSize[round+step])continue;    //
      else{score[i]=100-step;
           have_ideal=1;}
    } 
    if(have_ideal==0){   //迈大步,向心运动,不进毒圈，0
        int ideal2_0=0;int have_ideal2_0=0;
        int score2_0[moveSize]={0};
        for(int i=0;i<moveSize;i++){
            Coordinate tmp={pointsInMove[i].x,pointsInMove[i].y,pointsInMove[i].z};
            int dis_tmp=map.getDistance(tmp,pos);
            int dis_tmp2=map.getDistance(center,tmp);
            if(dis_tmp<move_range)continue;
            if(dis_tmp2>safeSize[round+1]+1)continue;   
            else{score2_0[i]=100-dis_tmp2;}
        }
        for(int i=0;i<moveSize;i++){
            if(score2_0[i]>have_ideal2_0){have_ideal2_0=score2_0[i];ideal2_0=i;}
        }
        Coordinate tmp2={pointsInMove[ideal2_0].x,pointsInMove[ideal2_0].y,pointsInMove[ideal2_0].z};
        op.target=tmp2;
    }
    else if(have_ideal==1){   //找到目标ideal
         for(int i=0;i<mine_size;i++){
             if(score[i]>have_ideal){
                 ideal=i;have_ideal=score[i];
             }
         }
         Coordinate tar=map.mine[ideal].pos;   //新版向目标进发
         int score3=100;
         int dis_3=0;
         int ideal_tmp2=0;
         for(int i=0;i<moveSize;i++){
            if(pointsInMove[i].BarrierIdx==0)continue;
            Coordinate tmp={pointsInMove[i].x,pointsInMove[i].y,pointsInMove[i].z};
            int dis_tmp=map.getDistance(tar,tmp);
            int dis_tmp2=map.getDistance(center,tmp);
            if(dis_tmp<score3||(dis_tmp==score3&&dis_tmp2<dis_3)){score3=dis_tmp;ideal_tmp2=i;dis_3=dis_tmp2;}
        }
        Coordinate tmp5={pointsInMove[ideal_tmp2].x,pointsInMove[ideal_tmp2].y,pointsInMove[ideal_tmp2].z};
        op.target=tmp5;
    }
}
else{
     //发现敌人或没有都一样
    if(enemy_num>0){
    enemy_history_pos=map.enemy_unit[0].pos;   
    enemy_move_range=map.enemy_unit[0].move_range;   
    enemy_attack_range=map.enemy_unit[0].attack_range;          
    enemy_sight_range=map.enemy_unit[0].sight_range;                
    danger=1;}
    int ideal3=0;
    int have_ideal3=0;
    int score[moveSize]={0};
    for(int i=0;i<moveSize;i++){
        Coordinate tmp={pointsInMove[i].x,pointsInMove[i].y,pointsInMove[i].z};
        int dis_tmp=map.getDistance(tmp,enemy_history_pos);
        if(dis_tmp<=enemy_attack_range)continue;
        int dis_tmp2=map.getDistance(tmp,pos);
        int dis_tmp3=map.getDistance(center,tmp);
        int step=(dis_tmp2-1)/move_range+1;if(dis_tmp2==0)step=0;
        if(dis_tmp3>safeSize[round+step])score[i]+=10;//安全
        else score[i]+=50;
        if(dis_tmp<=sight_range)score[i]+=30;//掌控
        for(int j=0;j<mine_size;j++){
           if(map.mine[j].belong==not_belong[id])continue;
            if(map.mine[j].num<=20)continue;
            if(map.mine[j].pos.x==tmp.x&&map.mine[j].pos.y==tmp.y&&map.mine[j].pos.z==tmp.z){score[i]+=20;break;}//矿藏
        }
        Coordinate tmp2={48-enemy_history_pos.x,48-enemy_history_pos.y,48-enemy_history_pos.z};
        int dis_tmp4=map.getDistance(tmp2,tmp);//周旋
        score[i]+=dis_tmp4;
    }
    for(int i=0;i<moveSize;i++){
        if(score[i]>have_ideal3){have_ideal3=score[i];ideal3=i;}
    }
    Coordinate tmp5={pointsInMove[ideal3].x,pointsInMove[ideal3].y,pointsInMove[ideal3].z};
    op.target=tmp5;
}
//升级系统
op.upgrade=0;
int up_size=19;
int up[19]={0,0,0,0,4,0,4,0,4,0,4,4,4,4,4,4,4,4,4};
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
        if(danger>0){
           if(enemy_move_range>move_range)
           { op.upgrade=1;op.upgrade_type=0;}
           else if(enemy_sight_range-2>sight_range)
           { op.upgrade=1;op.upgrade_type=4;}
           else if(up_num<up_size){
               if(mines>=UPGRADE_COST[up[up_num]]){ op.upgrade=1;op.upgrade_type=up[up_num];up_num++;}
           }
        }
       else{
          if(up_num<up_size){
               if(mines>=UPGRADE_COST[up[up_num]]){ op.upgrade=1;op.upgrade_type=up[up_num];up_num++;}
           }
       }
    }
}
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