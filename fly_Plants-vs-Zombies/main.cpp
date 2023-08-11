#include<stdio.h>
#include<graphics.h> //easyX图形库的头文件，需要安装easyX图形库
#include "tools.h"
#include "vector2.h"
#include<time.h>
#include<mmsystem.h>
#include<math.h>
#pragma comment(lib, "winmm.lib")

#define WIN_WIDTH 900
#define WIN_HEIGHT 600
#define ZM_MAX 10

enum{GOING, WIN, FAIL};
int killCount; //已经消灭的僵尸数量
int zmCount; //已经出现的僵尸总数
int gameStatus; //游戏的状态

enum{WAN_DOU, XIANG_RI_KUI, ZHI_WU_COUNT};

IMAGE imgBg;   //表示背景图片
IMAGE imgBar;  //道具栏图片
IMAGE imgCards[ZHI_WU_COUNT];  //道具栏上道具卡片
IMAGE* imgZhiWu[ZHI_WU_COUNT][20];  //种植后的植物

int curX, curY; //当前选中的植物在移动过程中的位置坐标
int curZhiWu; //当前选中哪个植物  0:表示没有选中  1：选择了第一种植物

struct zhiwu {
	int type;			//0：没有植物 1：选择了第一种植物
	int frameIndex;		//序列帧的序号
	bool catched; //被僵尸啃
	int deadTime; //死亡计数器
	int timer; //向日葵喷射阳光计时器
	int x, y; //植物坐标
	int shootTime;
};

struct zhiwu map[3][9];

enum { SUNSHINE_DOWN, SUNSHINE_GROUND, SUNSHINE_COLLECT, SUNSHINE_PRODUCT };

struct sunshineBall {
	int x, y;   //阳光球在飘落过程中的坐标位置（x不变）
	int frameIndex; //当前显示的图片帧的序号
	int destY; //飘落的目标位置的y坐标
	bool used; //是否在使用
	int timer;

	float xoff;  //被收集飞跃时每一帧移动量
	float yoff;

	float t; //贝塞尔曲线的时间点
	vector2 p1, p2, p3, p4;
	vector2 pCur; //当前时刻阳光球的位置
	float speed;
	int status;
};

struct sunshineBall balls[10];  //创建一个阳光池
IMAGE imgSunshineBall[29];  //飘落阳光球图片
int sunshine; //阳光值

struct zm {
	int x, y;
	int row;
	int frameIndex;
	bool used;
	int speed;
	int blood;
	bool dead;
	bool eating; //正在吃植物
};
struct zm zms[10];
IMAGE imgZM[22];
IMAGE imgZMDead[20];
IMAGE imgZMEat[21];
IMAGE imgZmStand[11];

//子弹的数据类型
struct bullet {
	int x, y;
	int row;
	bool used;
	int speed;
	bool blast; //是否发生爆炸
	int frameIndex; //爆炸效果帧序号
};
struct bullet bullets[30]; //子弹池
IMAGE imgBulletNormal;
IMAGE imgBullBlast[4];

//判断图片是否存在
bool fileExist(const char* name) {
	FILE* fp = fopen(name, "r");
	if (fp == nullptr) {
		return false;
	}
	else {
		fclose(fp);
		return true;
	}
}

//游戏初始化
void gameInit() {
	//加载有背景的图片
	//把字符集修改为“多字节字符集”
	loadimage(&imgBg, "res/bg.jpg");
	loadimage(&imgBar, "res/bar5.png");

	memset(imgZhiWu, 0, sizeof(imgZhiWu)); //指针全部设置为0
	memset(map, 0, sizeof(map));

	killCount = 0;
	zmCount = 0;
	gameStatus = GOING;

	//初始化植物卡牌和植物动态图片
	char name[64];
	for (int i = 0; i < ZHI_WU_COUNT; i++) {
		//生成植物卡牌的文件名
		sprintf_s(name, sizeof(name), "res/Cards/card_%d.png", i + 1);
		loadimage(&imgCards[i], name);
		for (int j = 0; i < 20; j++) {
			sprintf_s(name, sizeof(name), "res/zhiwu/%d/%d.png", i, j + 1);
			//先判断文件是否存在
			if (fileExist(name)) {
				imgZhiWu[i][j] = new IMAGE;
				loadimage(imgZhiWu[i][j], name);
			}
			else {
				break;
			}
		}
	}

	curZhiWu = 0;
	sunshine = 50;

	//初始化飘落的阳光球
	memset(balls, 0, sizeof(balls));
	for (int i = 0; i < 29; i++) {
		sprintf_s(name, sizeof(name), "res/sunshine/%d.png", i + 1);
		loadimage(&imgSunshineBall[i], name);
	}

	//配置随机种子
	srand(time(NULL));

	//创建游戏的图形窗口
	initgraph(WIN_WIDTH, WIN_HEIGHT);

	//设置字体
	LOGFONT f;
	gettextstyle(&f);
	f.lfHeight = 30;
	f.lfWeight = 15;
	strcpy(f.lfFaceName, "Segoe UI Black"); //设置字体类型
	f.lfQuality = ANTIALIASED_QUALITY; //抗锯齿效果
	settextstyle(&f);
	setbkmode(TRANSPARENT); //设置字体背景为透明
	setcolor(BLACK);

	//初始化僵尸图片
	memset(zms, 0, sizeof(zms));
	for (int i = 0; i < 22; i++) {
		sprintf_s(name, sizeof(name), "res/zm/%d.png", i);
		loadimage(&imgZM[i], name);
	}

	//初始化子弹图片
	loadimage(&imgBulletNormal, "res/bullets/bullet_normal.png");
	memset(bullets, 0, sizeof(bullets));
	
	//初始化豌豆子弹爆炸的图片帧数组
	loadimage(&imgBullBlast[3], "res/bullets/bullet_blast.png");
	for (int i = 0; i < 3; i++) {
		float k = (i + 1) * 0.2;
		loadimage(&imgBullBlast[i], "res/bullets/bullet_blast.png", imgBullBlast[3].getwidth() * k, imgBullBlast[3].getheight() * k, true);

	}

	//初始化僵尸死亡的图片帧数组
	for (int i = 0; i < 20; i++) {
		sprintf_s(name, sizeof(name), "res/zm_dead/%d.png", i + 1);
		loadimage(&imgZMDead[i], name);
	}

	//初始化僵尸吃植物图片帧数组
	for (int i = 0; i < 21; i++) {
		sprintf_s(name, sizeof(name), "res/zm_eat/%d.png", i + 1);
		loadimage(&imgZMEat[i], name);
	}

	//初始化僵尸站立的图片
	for (int i = 0; i < 11; i++) {
		sprintf_s(name, sizeof(name), "res/zm_stand/%d.png", i + 1);
		loadimage(&imgZmStand[i], name);
	}
}

void drawZM() {
	int zmCount = sizeof(zms) / sizeof(zms[0]);
	for (int i = 0; i < zmCount; i++) {
		if (zms[i].used) {
			//IMAGE* img = &imgZM[zms[i].frameIndex];
			//IMAGE* img = (zms[i].dead) ? imgZMDead : imgZM;
			IMAGE* img = NULL;
			if (zms[i].dead) img = imgZMDead;
			else if (zms[i].eating) img = imgZMEat;
			else img = imgZM;
			img += zms[i].frameIndex;
			putimagePNG(zms[i].x, zms[i].y - img->getheight(), img);
		}
	}
}

void drawSunshines() {
	//渲染飘落的阳光球
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < ballMax; i++) {
		//if (balls[i].used || balls[i].xoff) {
		if(balls[i].used){
			IMAGE* img = &imgSunshineBall[balls[i].frameIndex];
			//putimagePNG(balls[i].x, balls[i].y, img);
			putimagePNG(balls[i].pCur.x, balls[i].pCur.y, img);
		}
	}

	//渲染阳光值
	char scoreText[8];
	sprintf_s(scoreText, sizeof(scoreText), "%d", sunshine);
	outtextxy(285, 67, scoreText);
}

void drawCards() {
	for (int i = 0; i < ZHI_WU_COUNT; i++) {
		int x = 338 + i * 65;
		int y = 6;
		putimagePNG(x, y, &imgCards[i]);
	}
}

void drawZhiWu() {
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type > 0) {
				//int x = 256 + j * 81;
				//int y = 179 + i * 102 + 14;
				int zhiWuType = map[i][j].type - 1;
				int index = map[i][j].frameIndex;
				//putimagePNG(x, y, imgZhiWu[zhiWuType][index]);
				putimagePNG(map[i][j].x, map[i][j].y, imgZhiWu[zhiWuType][index]);
			}
		}
	}

	//渲染拖动过程中的植物
	if (curZhiWu > 0) {
		IMAGE* img = imgZhiWu[curZhiWu - 1][0];
		putimagePNG(curX - img->getwidth() / 2, curY - img->getheight() / 2, img);
	}
}

void drawBullets() {
	int bulletMax = sizeof(bullets) / sizeof(bullets[0]);
	for (int i = 0; i < bulletMax; i++) {
		if (bullets[i].used) {
			if (bullets[i].blast) {
				IMAGE* img = &imgBullBlast[bullets[i].frameIndex];
				putimagePNG(bullets[i].x, bullets[i].y, img);
			}
			else {
				putimagePNG(bullets[i].x, bullets[i].y, &imgBulletNormal);
			}
		}
	}
}

//渲染（把图片加载到窗口上）
void updateWindow() {
	BeginBatchDraw();//开始缓冲

	//渲染游戏背景图片
	putimage(-112, 0, &imgBg);   

	//putimagePNG:图片透明化，去除黑色背景。渲染道具栏图片
	putimagePNG(250, 0, &imgBar);  

	//渲染道具卡片
	drawCards();
	
	//渲染植物
	drawZhiWu();
		
	//渲染阳光
	drawSunshines(); 
	
	//渲染僵尸
	drawZM(); 

	//渲染豌豆子弹
	drawBullets();
	
	EndBatchDraw();//结束双缓冲   防止图片闪烁
}

void collectSunshine(ExMessage* msg) {
	int count = sizeof(balls) / sizeof(balls[0]);
	int w = imgSunshineBall[0].getwidth();
	int h = imgSunshineBall[0].getheight();
	for (int i = 0; i < count; i++) {
		if (balls[i].used) {
			//int x = balls[i].x;
			//int y = balls[i].y;
			int x = balls[i].pCur.x;
			int y = balls[i].pCur.y;

			if (msg->x > x && msg->x<x + w && msg->y>y && msg->y < y + h) {
				//balls[i].used = false;
				balls[i].status = SUNSHINE_COLLECT;
				//sunshine += 25;
				mciSendString("play res/sunshine.mp3", 0, 0, 0);
				//PlaySound("res/sunshine.wav", NULL, SND_FILENAME | SND_ASYNC);	//解决点击过快没音效，要使用WAV格式的音乐
				//设置阳光球的偏移量
				/*float destY = 0;
				float destX = 262;
				float angle = atan((y - destY) / (x - destX));
				balls[i].xoff = 4 * cos(angle);
				balls[i].yoff = 4 * sin(angle);*/
				balls[i].p1 = balls[i].pCur;
				balls[i].p4 = vector2(262, 0);
				balls[i].t = 0;
				float distance = dis(balls[i].p1 - balls[i].p4);
				float off = 16;
				balls[i].speed = 1.0 / (distance / off);
				break;
			}
		}
	}
}

//玩家点击操作
void userClick() {
	ExMessage msg;
	static int status = 0;
	if (peekmessage(&msg)) {   //判断有没有消息，有消息为真，没消息为假
		if (msg.message == WM_LBUTTONDOWN) {
			if (msg.x > 338 && msg.x < 338 + 65 * ZHI_WU_COUNT && msg.y < 96) {
				int index = (msg.x - 338) / 65;
				status = 1;
				curZhiWu = index + 1;
			}
			else {
				collectSunshine(&msg);
			}
		}
		else if (msg.message == WM_MOUSEMOVE && status == 1) {
			curX = msg.x;
			curY = msg.y;
		}
		else if (msg.message == WM_LBUTTONUP && status == 1) {
			if (msg.x > 256 - 112 && msg.y > 179 && msg.y < 489) {
				int row = (msg.y - 179) / 102;
				int col = (msg.x - (256 - 112)) / 81;

				if (map[row][col].type == 0) {
					map[row][col].type = curZhiWu;
					map[row][col].frameIndex = 0;
					map[row][col].shootTime = 0;
					map[row][col].x = 256 - 112 + col * 81;
					map[row][col].y = 179 + row * 102 + 14;
				}
			}
			curZhiWu = 0;
			status = 0;
		}
	}
}

void createSunshine() {
	static int count = 0;
	static int fre = 400;
	count++;
	//自然飘落阳光
	if (count >= fre) {
		fre = 200 + rand() % 200;
		count = 0;

		//从阳光池中取一个可以使用的
		int ballMax = sizeof(balls) / sizeof(balls[0]);
		int i;
		for (i = 0; i < ballMax && balls[i].used; i++);
		if (i >= ballMax) return;
		balls[i].used = true;
		balls[i].frameIndex = 0;
		//balls[i].x = 260 + rand() % (800 - 260); //260-900
		//balls[i].y = 60;
		//balls[i].destY = 200 + (rand() % 4) * 90;
		balls[i].timer = 0;
		/*balls[i].xoff = 0;
		balls[i].yoff = 0;*/
		balls[i].status = SUNSHINE_DOWN;
		balls[i].t = 0;
		balls[i].p1 = vector2(260-112 + rand() % (800 - 260+112), 60);
		balls[i].p4 = vector2(balls[i].p1.x, 200 + (rand() % 4) * 90);
		int off = 2;
		float distance = balls[i].p4.y - balls[i].p1.y;
		balls[i].speed = 1.0 / (distance / off);
	}

	//向日葵生产阳光
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type == XIANG_RI_KUI+1) {
				map[i][j].timer++;
				if (map[i][j].timer > 200) {
					map[i][j].timer = 0;
					int k;
					for (k = 0; k < ballMax&& balls[k].used; k++);
					if (k > ballMax)return;
					balls[k].used = true;
					balls[k].p1 = vector2(map[i][j].x, map[i][j].y);
					int w = (50 + rand() % 50) * (rand() % 2 ? 1 : -1);
					balls[k].p4 = vector2(map[i][j].x + w,
						map[i][j].y + imgZhiWu[XIANG_RI_KUI][0]->getheight() -
						imgSunshineBall[0].getheight());
					balls[k].p2 = vector2(balls[k].p1.x + w * 0.3, balls[k].p1.y - 100);
					balls[k].p3 = vector2(balls[k].p1.x + w * 0.7, balls[k].p1.y - 150);
					balls[k].status = SUNSHINE_PRODUCT;
					balls[k].speed = 0.05;
					balls[k].t = 0;
				}
			}
		}
	}
}

void updateSunshine() {
	int ballMax = sizeof(balls) / sizeof(balls[0]);
	for (int i = 0; i < ballMax; i++) {
		if (balls[i].used) {
			balls[i].frameIndex = (balls[i].frameIndex + 1) % 29;
			if (balls[i].status == SUNSHINE_DOWN) {
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->pCur = sun->p1 + sun->t * (sun->p4 - sun->p1);
				if (sun->t >= 1) {
					sun->status = SUNSHINE_GROUND;
					sun->t = 0;
					sun->timer = 0;
				}
			}
			else if (balls[i].status == SUNSHINE_GROUND) {
				balls[i].timer++;
				if (balls[i].timer > 100) {
					balls[i].used = false;
					balls[i].timer = 0;
				}
			}
			else if (balls[i].status == SUNSHINE_COLLECT) {
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->pCur = sun->p1 + sun->t * (sun->p4 - sun->p1);
				if (sun->t >= 1) {
					sun->used = false;
					sunshine += 25;
					sun->t = 0;
				}
			}
			else if (balls[i].status == SUNSHINE_PRODUCT) {
				struct sunshineBall* sun = &balls[i];
				sun->t += sun->speed;
				sun->pCur = calcBezierPoint(sun->t, sun->p1, sun->p2, sun->p3, sun->p4);
				if (sun->t >= 1) {
					sun->status = SUNSHINE_GROUND;
					sun->timer = 0;
					sun->t = 0;
				}
			}

			//	balls[i].frameIndex = (balls[i].frameIndex + 1) % 29;
			//	if (balls[i].timer == 0) {
			//		balls[i].y += 2;
			//	}
			//	if (balls[i].y >= balls[i].destY) {
			//		balls[i].timer++;
			//		if (balls[i].timer > 100) {
			//			balls[i].used = false;
			//		}
			//	}
			//}
			//else if (balls[i].xoff) {
			//	//实时更新偏移值
			//	float destY = 0;
			//	float destX = 262;
			//	float angle = atan((balls[i].y - destY) / (balls[i].x - destX));
			//	balls[i].xoff = 4 * cos(angle);
			//	balls[i].yoff = 4 * sin(angle);
			//	balls[i].x -= balls[i].xoff;
			//	balls[i].y -= balls[i].yoff;
			//	if (balls[i].y < 0 || balls[i].x < 260) {
			//		balls[i].xoff = 0;
			//		balls[i].yoff = 0;
			//		sunshine += 25;
			//	}
			//}
		}
	}
}
//创建僵尸
void createZM() {
	if (zmCount >= ZM_MAX) {
		return;
	}

	static int zmFre = 300;
	static int count;
	count++;
	if (count > zmFre) {
		count = 0;
		zmFre = rand() % 300 + 200;
		int i;
		int zmMax = sizeof(zms) / sizeof(zms[0]);
		for (i = 0; i < zmMax && zms[i].used; i++);
		if (i < zmMax) {
			zms[i].used = true;
			zms[i].x = WIN_WIDTH;
			zms[i].row = rand() % 3; //0-2
			zms[i].y = 172 + (1 +zms[i].row) * 100;
			zms[i].speed = 1;
			zms[i].blood = 100;
			zms[i].dead = false;
			zms[i].eating = false;
			zmCount++;
		}
	}
}

void updateZM() {
	int zmMax = sizeof(zms) / sizeof(zms[0]);
	static int count = 0;
	count++;
	if (count > 2) {
		count = 0;
		//更新僵尸的位置
		for (int i = 0; i < zmMax; i++) {
			if (zms[i].used) {
				zms[i].x -= zms[i].speed;
				if (zms[i].x < -64) {
					//printf("GAME OVER\n");
					//MessageBox(NULL, "over", "over", 0); //待优化
					//exit(0); //待优化
					gameStatus = FAIL;
				}
			}
		}
	}

	static int count2 = 0;
	count2++;
	if (count2 > 2) {
		count2 = 0;
		for (int i = 0; i < zmMax; i++) {
			if (zms[i].used) {
				if (zms[i].dead) {
					zms[i].frameIndex++;
					if (zms[i].frameIndex >= 20) {
						zms[i].used = false;
						killCount++;
						if (killCount == ZM_MAX) {
							gameStatus = WIN;
						}
					}
				}
				else if (zms[i].eating) {
					zms[i].frameIndex = (zms[i].frameIndex + 1) % 21;
				}
				else zms[i].frameIndex = (zms[i].frameIndex + 1) % 22;
			}
		}
	}	
}

void shoot() {
	int lines[3] = { 0 };
	int zmCount = sizeof(zms) / sizeof(zms[0]);
	int bulletMax = sizeof(bullets) / sizeof(bullets[0]);
	int dangerX = WIN_WIDTH - imgZM[0].getwidth() / 2;
	for (int i = 0; i < zmCount; i++) {
		if (zms[i].used && zms[i].x < dangerX) {
			lines[zms[i].row] = 1;
		}
	}
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type == WAN_DOU + 1&&lines[i]) {
				//static int count = 0;
				//count++;
				map[i][j].shootTime++;
				if (map[i][j].shootTime > 40) {
					map[i][j].shootTime = 0;
					int k;
					for (k = 0; k < bulletMax && bullets[k].used; k++);
					if (k < bulletMax) {
						bullets[k].used = true;
						bullets[k].row = i;
						bullets[k].speed = 4;
						bullets[k].blast = false;
						bullets[k].frameIndex = 0;
						int zwX = 256 - 112 + j * 81;
						int zwY = 179 + i * 102 + 14;
						bullets[k].x = zwX + imgZhiWu[map[i][j].type - 1][0]->getwidth() - 10;
						bullets[k].y = zwY + 5;
					}
				}
			}
		}
	}
}

void updateBullets() {
	int countMax = sizeof(bullets) / sizeof(bullets[0]);
	for (int i = 0; i < countMax; i++) {
		if (bullets[i].used) {
			bullets[i].x += bullets[i].speed;
			if(bullets[i].x > WIN_WIDTH){
				bullets[i].used = false;
			}
			if (bullets[i].blast) {
				bullets[i].frameIndex++;
				if (bullets[i].frameIndex > 3) {
					bullets[i].used = false;
				}
			}
		}
	}
}

//检测子弹触碰僵尸
void checkBullet2Zm() {
	int bCount = sizeof(bullets) / sizeof(bullets[0]);
	int zCount = sizeof(zms) / sizeof(zms[0]);
	for (int i = 0; i < bCount; i++) {
		if (bullets[i].used == false || bullets[i].blast)continue;
		for (int k = 0; k < zCount; k++) {
			if (zms[k].used == false)continue;
			int x1 = zms[k].x + 80;
			int x2 = zms[k].x + 180;
			int x = bullets[i].x;
			if (zms[k].dead == false && bullets[i].row == zms[k].row && x > x1 && x < x2) {
				zms[k].blood -= 10;
				bullets[i].blast = true;
				bullets[i].speed = 0;
				if (zms[k].blood <= 0) {
					zms[k].dead = true;
					zms[k].speed = 0;
					zms[k].frameIndex = 0;
				}
				break;
			}
		}
	}
}

void checkZm2ZhiWu() {
	int zCount = sizeof(zms) / sizeof(zms[0]);
	for (int i = 0; i < zCount; i++) {
		if (zms[i].dead) continue;
		int row = zms[i].row;
		for (int k = 0; k < 9; k++) {
			if (map[row][k].type == 0)continue;
			int zhiWuX = 256 - 112 + k * 81;
			int x1 = zhiWuX + 10;
			int x2 = zhiWuX + 60;
			int x3 = zms[i].x + 80;
			if (x3 > x1 && x3 < x2) {
				if (map[row][k].catched) {
					map[row][k].deadTime++;
					if (map[row][k].deadTime > 100) {
						map[row][k].deadTime = 0;
						map[row][k].type = 0;
						zms[i].eating = false;
						zms[i].frameIndex = 0;
						zms[i].speed = 1;
					}
				}
				else {
					map[row][k].catched = true;
					map[row][k].deadTime = 0;
					zms[i].eating = true;
					zms[i].speed = 0;
					zms[i].frameIndex = 0;
				}
			}
		}
	}
}

void collisionCheck() {
	checkBullet2Zm(); //子弹对僵尸碰撞检测
	checkZm2ZhiWu(); //僵尸对植物碰撞检测
}

//已经种植的植物摆动
void updateZhiWu() {
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 9; j++) {
			if (map[i][j].type > 0) {
				map[i][j].frameIndex++;
				int zhiWuType = map[i][j].type - 1;
				int index = map[i][j].frameIndex;
				if (imgZhiWu[zhiWuType][index] == nullptr) {
					map[i][j].frameIndex = 0;
				}
			}
		}
	}
}

//实时更新游戏数据
void updateGame() {

	updateZhiWu(); //已经种植的植物摆动

	createSunshine(); //创建阳光
	updateSunshine(); //更新阳光状态

	createZM(); //创建行走僵尸
	updateZM(); //更新行走僵尸状态

	shoot(); //发射豌豆子弹
	updateBullets(); //更新豌豆子弹

	collisionCheck(); //豌豆子弹碰撞检测
}

//启动UI界面
void startUI(){
	IMAGE imgBg, imgMenu1, imgMenu2;
	loadimage(&imgBg, "res/menu.png");
	loadimage(&imgMenu1, "res/menu1.png");
	loadimage(&imgMenu2, "res/menu2.png");
	int flag = 0;
	while (1) {
		BeginBatchDraw();
		putimage(0, 0, &imgBg);
		putimagePNG(474, 75, flag ? &imgMenu2 : &imgMenu1);

		ExMessage msg;
		if (peekmessage(&msg)) {
			if (msg.message == WM_LBUTTONDOWN && msg.x > 474 && msg.x < 474 + 300 && msg.y>75 && msg.y < 75 + 140) {
				flag = 1;
			}
			else if (msg.message == WM_LBUTTONUP && flag) {
				EndBatchDraw();
				break;
			}
		}

		EndBatchDraw();
	}
}

void viewScence() {
	int xMin = WIN_WIDTH - imgBg.getwidth();
	vector2 points[9] = {
		{550,80},{530,160},{630,170},{530,200},{515,270},
		{565,370},{605,340},{705,280},{690,340}
	};
	int index[9];
	for (int i = 0; i < 9; i++) {
		index[i] = rand() % 11;
	}

	int count = 0;
	for (int x = 0; x >= xMin; x -= 4) {
		BeginBatchDraw();
		putimagePNG(x, 0, &imgBg);
		count++;
		for (int k = 0; k < 9; k++) {
			putimagePNG(points[k].x - xMin + x, points[k].y, &imgZmStand[index[k]]);
			if (count >= 5) {
				index[k] = (index[k] + 1) % 11;
			}
		}
		if (count >= 10)count = 0;

		EndBatchDraw();
		Sleep(5);
	}
	//停留1s左右
	for (int i = 0; i < 50; i++) {
		BeginBatchDraw();

		putimagePNG(xMin, 0, &imgBg);
		for (int k = 0; k < 9; k++) {
			putimagePNG(points[k].x, points[k].y, &imgZmStand[index[k]]);
			index[k] = (index[k] + 1) % 11;
		}

		EndBatchDraw();
		Sleep(30);
	}

	for (int x = xMin; x <= -112; x += 4) {
		BeginBatchDraw();
		putimagePNG(x, 0, &imgBg);
		count++;
		for (int k = 0; k < 9; k++) {
			putimagePNG(points[k].x - xMin + x, points[k].y, &imgZmStand[index[k]]);
			if (count >= 5) {
				index[k] = (index[k] + 1) % 11;
			}
		}
		if (count >= 10)count = 0;
		EndBatchDraw();
		Sleep(5);
	}

}

void barsDown() {
	int height = imgBar.getheight();
	for (int y = -height; y <= 0; y++) {
		BeginBatchDraw();
		putimagePNG(-112, 0, &imgBg);
		putimagePNG(250, y, &imgBar);
		for (int i = 0; i < ZHI_WU_COUNT; i++) {
			int x = 338 + i * 65;
			putimagePNG(x, 6+y, &imgCards[i]);
		}
		EndBatchDraw();
		Sleep(5);
	}
	
}

bool checkOver() {
	int ret = false;
	if (gameStatus == WIN) {
		Sleep(2000);
		loadimage(0, "res/gameWin.png");
		mciSendString("play res/win.mp3", 0, 0, 0);
		ret = true;
	}
	else if (gameStatus == FAIL) {
		Sleep(2000);
		loadimage(0, "res/gameFail.png");
		mciSendString("play res/lose.mp3", 0, 0, 0);
		ret = true;
	}
	return ret;
}

int main() {
	gameInit();  //游戏初始化

	startUI();   //加载游戏开始界面

	viewScence(); //片头巡场

	barsDown();

	int timer = 0;
	bool flag = true;
	while (1) {
		userClick();

		timer += getDelay();   //获取累计延迟时间
		if (timer > 20) {
			flag = true;
			timer = 0;
		}
		if (flag) {
			flag = false;
			updateWindow();  //渲染画面
			updateGame();  //实时改变游戏内的数据
			if(checkOver()) break; //检查游戏是否结束
		}

		
	}
	

	system("pause");
	return 0;
}
