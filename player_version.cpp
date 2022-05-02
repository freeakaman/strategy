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
int player.player.player.player.player.player.hp 生命值
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
static int turn_num=0;
/*请修改本函数*/
Operation get_operation(const Player& player, const Map& map)
{
    getInfo(player, map);
    turn_num++;
    Operation op;
    Coordinate pos=player.pos;     
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
   
    if(hp<=50||turn_num==100||turn_num==99){//血量见底，回血  （回合数见底）
        if(mines>=40){
             op.upgrade=1; op.upgrade_type=3;    
        }
        else{
             op.upgrade=0;
        }
    }
    else{  //血量还可以
        if(mine_speed<30){  //挖矿速度未达满
            if(mines<5){
             op.upgrade=0;   //其实可以利用犯规机制
            }
            else{
              op.upgrade=1; op.upgrade_type=2;       
            }
        }
        else{
           op.upgrade=0;//攒着最后拼血
        }
    }
    op.target.x=0;
    op.target.y=0;
    op.target.z=0;
    op.type=0;
    int min=mine_speed;if(min>20)min=20; //min
    int not_belong[2]={-1,1};
    //如果正站在资源点上
    int judge_on_mine=0;int mine_i=-1;
    for(int i=0;i<map.mine.size();i++){
     if(map.mine[i].pos.x==x&&map.mine[i].pos.y==y&&map.mine[i].pos.z==z){  //未重载==pos
      if(map.mine[i].belong!=not_belong[id]){
           judge_on_mine=1;mine_i=i;
           break;}
           }
           }
    if(judge_on_mine==1&&map.mine[mine_i].num>=min){
      //如果资源还足够多，就不动弹,打人！
      op.type=1;
    }
    //未完成的任务：我先见敌，敌先见我，公共资源点争夺，实力比较
    if(op.type==1){
      if(map.enemy_num!=0)
        op.target=map.enemy_unit[0].pos;return op;
    }
    else{
      int mine_num=0;
      for(int i=0;i<map.mine.size();i++){
          if(map.mine[i].belong!=not_belong[id]&&map.mine[i].num>=min){
              mine_num++;
          }
      }
      op.type=0;
      if(mine_num==0){   //向心运动
          Coordinate center={24,24,24};
          int jud2,jud3;
          jud2=map.getDistance(center,pos);  //map.getDistance(tmp, player.pos)
          for(int i=0;i<pointsInMove.size();i++){
              if(pointsInMove[i].BarrierIdx==0)continue;
              Coordinate tmp={pointsInMove[i].x,pointsInMove[i].y,pointsInMove[i].z};
              int jud3=map.getDistance(tmp,center);
              if(jud3<jud2){op.target=tmp;return op;}
          }
              for(int i=0;i<pointsInMove.size();i++){
              if(pointsInMove[i].BarrierIdx==0)continue;
              Coordinate tmp={pointsInMove[i].x,pointsInMove[i].y,pointsInMove[i].z};
              int jud3=map.getDistance(tmp,center);
              if(jud3==jud2){op.target=tmp;return op;}
          }
              for(int i=0;i<pointsInMove.size();i++){
              if(pointsInMove[i].BarrierIdx==-1)
              {Coordinate tmp={pointsInMove[i].x,pointsInMove[i].y,pointsInMove[i].z};op.target=tmp;return op;}
          } 
      }
      //如果视野内有资源，编写权衡取舍函数
      else{
          int ideal_mine=0;
          Coordinate tar={0,0,0};
          int dis_ideal=100;
          for(int i=0;i<map.mine.size();i++){
            if(map.mine[i].belong!=not_belong[id]&&map.mine[i].num>=min){
               if(map.getDistance(pos,map.mine[i].pos)<dis_ideal){
                   tar.x=map.mine[i].pos.x;
                   tar.y=map.mine[i].pos.y;
                   tar.z=map.mine[i].pos.z;
                   dis_ideal=map.getDistance(pos,map.mine[i].pos);
                   ideal_mine=1;
               }
            }
          }
          //如果没相中的，向心
          if(ideal_mine==0){
             Coordinate center={24,24,24};
            int jud2,jud3;
             jud2=map.getDistance(pos,center);
            for(int i=0;i<pointsInMove.size();i++){
              if(pointsInMove[i].BarrierIdx==0)continue;
              Coordinate tmp={pointsInMove[i].x,pointsInMove[i].y,pointsInMove[i].z};
              int jud3=map.getDistance(tmp,center);
              if(jud3<jud2){op.target=tmp;return op;}
          }
              for(int i=0;i<pointsInMove.size();i++){
              if(pointsInMove[i].BarrierIdx==0)continue;
              Coordinate tmp={pointsInMove[i].x,pointsInMove[i].y,pointsInMove[i].z};
              int jud3=map.getDistance(tmp,center);
              if(jud3==jud2){op.target=tmp;return op;}
          }
              for(int i=0;i<pointsInMove.size();i++){
              if(pointsInMove[i].BarrierIdx==-1)
              {Coordinate tmp={pointsInMove[i].x,pointsInMove[i].y,pointsInMove[i].z};op.target=tmp;return op;}
          } 
        }
          //否则，向目标前进
          else{
              
              for(int i=0;i<pointsInMove.size();i++){
                 if(pointsInMove[i].BarrierIdx==0)continue;
                 Coordinate tmp={pointsInMove[i].x,pointsInMove[i].y,pointsInMove[i].z};
                 int dis2=map.getDistance(tar,tmp);
                 if(dis2<dis_ideal){op.target=tmp;return op;}
              }
               for(int i=0;i<pointsInMove.size();i++){
                 if(pointsInMove[i].BarrierIdx==0)continue;
                 Coordinate tmp={pointsInMove[i].x,pointsInMove[i].y,pointsInMove[i].z};
                 int dis2=map.getDistance(tar,tmp);
                 if(dis2==dis_ideal){op.target=tmp;return op;}
              }
              for(int i=0;i<pointsInMove.size();i++){
                 if(pointsInMove[i].BarrierIdx==-1){ Coordinate tmp={pointsInMove[i].x,pointsInMove[i].y,pointsInMove[i].z};
                 op.target=tmp;return op;}
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
//this is a test for my git
//this haha