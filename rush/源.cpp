#include<stdio.h>					
#include <stdlib.h>									
#include <time.h>						
#include<string.h>

//棋盘信息
#define N_PIECES 12									//12个子										
#define N_BOARD 8								//8*8棋盘
#define N_STEP 13							//最大13步
#define N_COMMAND 25				//一个局面最多25可走法吧
#define EMPTY -1											//-1死了，0普通，1升变
#define NORMAL 0										
#define SUPER 1
#define BLACK 0										//黑0白1，输入的是黑1白2，减1就好了
#define WHITE 1					
#define DOUBLE(a) (2*a)				//加倍
#define START 1						//用来判断命令用的
#define TURN 2
#define PLACE 3
#define END 4
#define INFINITY 10000				//无穷大，ab用
#define ANOTHERFLAG(x) (1-x)									//另一方
#define TRUE 1
#define FALSE 0


//以下是变量与结构体
int g_cut_time;									//截断时间
int g_find_depth;							//搜索深度层
int g_super_score;									//超子的分
int g_normal_score;								//普通子的分
typedef int FLAG;
typedef int BOOL;								//记录回合,START记了一轮故为0
const int direction[4][2] = { {-1,-1},{-1,1},{1,-1},{1,1} };	//黑普通搜0，1。白普通搜2，3
int myflag, enemyflag;
int g_t_start, g_t_end;
BOOL depth_is_odd;												//记录要搜奇数层偶数层

struct PieceMode			//子态存储
{
	int mode;			//-1死0普通1超
	int x;			//记录位置
	int y;
};

struct Command
{
	int x[N_STEP];
	int y[N_STEP];
	int num_step;
};

struct AllCommand					//findEat和findMove的结果，主要因为findEat递归需要全局储存所以设立
{
	struct Command all_cmd[N_COMMAND];
}g_all_cmd;

struct Board						//棋盘信息
{
	char map[N_BOARD][N_BOARD];	//棋盘上的黑子从0到11，白子从12到23
	struct PieceMode pieces[2][N_PIECES];	//0黑1白
	struct Command best_cmd;			//局面最优解
}g_board;			//当前棋盘上的局面

/**
 * YOUR CODE BEGIN
 * 你的代码开始
 */

 //以下是一些函数
BOOL isInside(int x, int y)				//在不在棋盘内
{
	if (x >= 0 && x < N_BOARD)
		if (y >= 0 && y < N_BOARD)
			if ((x + y) % 2)
				return 1;
	return 0;
}

FLAG flagJudge(int num)				//棋盘上找子用，判断子是黑是白
{
	if (num != EMPTY)
	{
		int tmp = num / N_PIECES;
		return tmp;
	}
	else
		return -1;
}

void showBrd(char map[8][8])
{
	printf("\n");
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			switch (flagJudge(map[i][j]))
			{
			case EMPTY:
				printf(".");
				break;
			case BLACK:
				printf("B");
				break;
			case WHITE:
				printf("W");
				break;
			}
		}
		printf("\n");
	}
}

int countOne(struct Board brd, struct PieceMode tmp, int flag)
{
	if (tmp.mode == EMPTY)
		return 0;
	int score = 0;
	int dir_start, dir_end;
	if (tmp.mode)
	{
		score += g_super_score;
		dir_start = 0, dir_end = 4;
	}
	else
	{
		score += g_normal_score;
		dir_start = flag * 2, dir_end = flag * 2 + 2;
	}
	for (int j = 0; j < 4; j++)
	{
		int judgex = tmp.x + direction[j][0];
		int	judgey = tmp.y + direction[j][1];
		if (isInside(judgex, judgey))
			if (brd.map[judgex][judgey] == EMPTY)
				score++;
	}
	return score;
}

//估值函数
int countAll(struct Board tmp)					//估值永远是全局我方值减对方值
{
	int my_score = 0, enemy_score = 0;
	for (int i = 0; i < N_PIECES; i++)
	{
		my_score += countOne(tmp, tmp.pieces[myflag][i], myflag);
		enemy_score += countOne(tmp, tmp.pieces[enemyflag][i], enemyflag);
	}
	if (my_score == 0)						//我输了
		return -INFINITY;
	else if (enemy_score == 0)					//我赢了
		return INFINITY;
	if (depth_is_odd)				//奇偶估值不一样
		return -(my_score - enemy_score);
	else
		return my_score - enemy_score;
}

int mid(int x1, int x2)				//中值
{
	if (abs(x1 - x2) == 1)
		return 0;
	else
		return (int)((x1 + x2) / 2);
}

struct Board move(struct Board in, FLAG flag, struct Command command)			//落子函数，flag表示是谁下的，0黑1白
{
	int piecenum = in.map[command.x[0]][command.y[0]];
	in.pieces[flag][piecenum % N_PIECES].x = command.x[command.num_step - 1];
	in.pieces[flag][piecenum % N_PIECES].y = command.y[command.num_step - 1];
	in.map[command.x[0]][command.y[0]] = EMPTY;
	in.map[command.x[command.num_step - 1]][command.y[command.num_step - 1]] = piecenum;
	if (flag == 0 && command.x[command.num_step - 1] == 0)								//黑，行为0，升变
		in.pieces[flag][piecenum % N_PIECES].mode = 1;
	if (flag == 1 && command.x[command.num_step - 1] == 7)								//白,行为7，升变
		in.pieces[flag][piecenum % N_PIECES].mode = 1;										//白升变
	for (int i = 0; i < (command.num_step - 1); i++)
	{
		int midx = mid(command.x[i], command.x[i + 1]);
		int midy = mid(command.y[i], command.y[i + 1]);
		if (midx&&midy)
		{
			int tmpnum = in.map[midx][midy];
			FLAG anotherflag = flag ? 0 : 1;
			in.pieces[anotherflag][tmpnum % N_PIECES].mode = EMPTY;
			in.map[midx][midy] = EMPTY;
		}
	}
	return in;
}

//可吃搜索
void findEat(struct Board temp, int x, int y, int flag, struct Command all_cmd)		//搜索并存储可吃commmand，储存在g_all_cmd
{													//findEat需要一个不断进去的command，一个全局储存的command数组
	all_cmd.x[all_cmd.num_step] = x;
	all_cmd.y[all_cmd.num_step] = y;
	all_cmd.num_step++;
	int piecenum = temp.map[x][y];
	for (int i = 0; i < 4; i++)
	{
		int togox = x + DOUBLE(direction[i][0]);
		int togoy = y + DOUBLE(direction[i][1]);
		if (isInside(togox, togoy))
		{
			int toeatx = x + direction[i][0];
			int toeaty = y + direction[i][1];
			if (flagJudge(temp.map[toeatx][toeaty]) == ANOTHERFLAG(flag))
				if (temp.map[togox][togoy] == EMPTY)
				{
					temp.map[togox][togoy] = piecenum;
					temp.map[x][y] = EMPTY;
					temp.map[toeatx][toeaty] = EMPTY;
					findEat(temp, togox, togoy, flag, all_cmd);
				}
		}
	}
	if (all_cmd.num_step > g_all_cmd.all_cmd[0].num_step)					//找并存,存在g_all_cmd内
	{
		memset(&g_all_cmd, 0, sizeof(g_all_cmd));
		g_all_cmd.all_cmd[0] = all_cmd;
	}
	else if (all_cmd.num_step == g_all_cmd.all_cmd[0].num_step)
		for (int i = 1; i < N_COMMAND; i++)
			if (all_cmd.num_step > g_all_cmd.all_cmd[i].num_step)
			{
				g_all_cmd.all_cmd[i] = all_cmd;
				break;
			}
}

//可走搜索
void findMove(struct Board datatmp, int flag)		//思路同上，找并存入g_all_cmd
{
	struct Command all_cmd;
	for (int k = 0; k < N_PIECES; k++)								//12个子
	{
		int mode = datatmp.pieces[flag][k].mode;
		int dir_start, dir_end;
		if (mode == EMPTY)
			continue;
		if (mode)
			dir_start = 0, dir_end = 4;
		else
			dir_start = flag * 2, dir_end = flag * 2 + 2;
		memset(&all_cmd, 0, sizeof(all_cmd));
		int x = datatmp.pieces[flag][k].x;
		int y = datatmp.pieces[flag][k].y;
		all_cmd.x[0] = x;
		all_cmd.y[0] = y;
		all_cmd.num_step = 1;
		for (int i = dir_start; i < dir_end; i++)
		{
			int togox = x + direction[i][0];
			int togoy = y + direction[i][1];
			if (isInside(togox, togoy))
				if (datatmp.map[togox][togoy] == EMPTY)					//为空
				{
					all_cmd.x[1] = togox;
					all_cmd.y[1] = togoy;
					all_cmd.num_step = 2;
					for (int j = 0; j < N_COMMAND; j++)
					{
						if (g_all_cmd.all_cmd[j].num_step < 2)
						{
							g_all_cmd.all_cmd[j] = all_cmd;
							break;
						}
					}
				}
		}
	}
}

int commandNum(struct AllCommand all_cmd) 
{
	for (int n = 0; n < N_COMMAND; n++)
	{
		if (all_cmd.all_cmd[n].num_step < 2)
			return n;
	}
	return N_COMMAND;
}


int alphaBeta(struct Board *current,
	int alpha, int beta,
	int depth, FLAG flag)		//ab剪枝
{
	g_t_end = clock();									//计时，时间不够赶紧溜
	if (g_t_end - g_t_start >= g_cut_time)
		return alpha;
	if (depth == 0)										//0层估值,奇偶不一样
		return countAll(*current);
	memset(&g_all_cmd, 0, sizeof(g_all_cmd));
	struct Command all_cmd;
	memset(&all_cmd, 0, sizeof(all_cmd));
	for (int i = 0; i < N_PIECES; i++)
	{
		if (current->pieces[flag][i].mode != -1)
			findEat(*current,
				current->pieces[flag][i].x, current->pieces[flag][i].y,
				flag, all_cmd);
	}
	if (g_all_cmd.all_cmd[0].num_step < 2)
	{
		memset(&g_all_cmd, 0, sizeof(g_all_cmd));
		findMove(*current, flag);
	}																				//找完了可行路
	if (depth == g_find_depth && commandNum(g_all_cmd)==1)		//如果只有一步可以走，直接走，就不搜了
	{
		current->best_cmd = g_all_cmd.all_cmd[0];
		return 0;
	}
	struct AllCommand local_all_cmd = g_all_cmd;		//用个局部存起来，不然不断变会崩
	for (int i = 0; i < N_COMMAND; i++)
	{
		int val;
		if (local_all_cmd.all_cmd[i].num_step >= 2)
		{
			struct Board next = move(*current, flag, local_all_cmd.all_cmd[i]);
			val = -alphaBeta(&next,
				-beta, -alpha,
				depth - 1, ANOTHERFLAG(flag));
			if (val >= beta)										//局面没有更好的结果了，溜
			{
				current->best_cmd = local_all_cmd.all_cmd[i];		//担心估值到正无穷就不走了
				return beta;
			}
			if (val > alpha)												//更改当前局面最想要的结果
			{
				alpha = val;
				current->best_cmd = local_all_cmd.all_cmd[i];
			}
			if (val == alpha)								//有的时候全断了，总要给个command吧
				if (current->best_cmd.num_step < 2)
					current->best_cmd = local_all_cmd.all_cmd[i];
		}
		else
			return alpha;										//全搜完了就返回
	}
}

int returnX(int a)			//返回序数对应的行数，初始化的时候用
{
	a /= 4;
	return a;
}

int returnY(int a)			//返回序数对应的列数，初始化的时候用
{
	int c = ((returnX(a) % 2) ? 2 * (a % 4) : 2 * (a % 4) + 1);
	return c;
}

int orderCase(char tmp[])			//这是什么命令鸭
{
	if (!strcmp(tmp, "START"))
	{
		return START;
	}
	else if (!strcmp(tmp, "TURN"))
	{
		return TURN;
	}
	else if (!strcmp(tmp, "PLACE"))
	{
		return PLACE;
	}
	else if (!strcmp(tmp, "END"))
	{
		return END;
	}
}

/**
 * You can define your own struct and variable here
 * 你可以在这里定义你自己的结构体和变量
 */

 /**
  * 你可以在这里初始化你的AI
  */
void initAI()											//初始化
{
	memset(g_board.map, -1, sizeof(g_board.map));		//棋盘空空如也	
	for (int i = 0; i < N_PIECES; i++)
	{
		g_board.pieces[1][i].mode = 0;							//0黑1白
		g_board.pieces[1][i].x = returnX(i);
		g_board.pieces[1][i].y = returnY(i);
		g_board.map[returnX(i)][returnY(i)] = i + 12;
		g_board.pieces[0][i].mode = 0;
		g_board.pieces[0][i].x = returnX(i + 20);
		g_board.pieces[0][i].y = returnY(i + 20);
		g_board.map[returnX(i + 20)][returnY(i + 20)] = i;			//棋盘和模式互联
	}
}

/**
 * 轮到你落子。
 * 棋盘上0表示空白，1表示黑棋，2表示白旗
 * me表示你所代表的棋子(1或2)
 * 你需要返回一个结构体Command，其中num_step是你要移动的棋子经过的格子数（含起点、终点），
 * x、y分别是该棋子依次经过的每个格子的横、纵坐标
 */

void correctV(int f_tmp, int c_tmp, int n_tmp, int s_tmp)
{
	g_find_depth = f_tmp;
	g_cut_time = c_tmp;
	g_normal_score = n_tmp;
	g_super_score = s_tmp;
}

void correctVariate(int turn)				//矫正变量
{
	if (turn == 1 || turn == 2)											//不同阶段不同战略
		correctV(10, 1870, 5, 5);
	else if (turn >= 3 && turn <= 10)
		correctV(10, 1850, 5, 5);
	else if (turn >= 11 && turn <= 60)
		correctV(10, 1840, 5, 5);
	else if (turn >= 61 && turn <= 80)
		correctV(10, 1850, 5, 6);
	else if (turn >= 81 && turn < 90)
		correctV(10, 1830, 5, 9);
	else if (turn >= 91)
		correctV(10, 1910, 10, 30);
	if (120 - turn + 1 < 10)
		correctV(120 - turn + 1, 1890, 10, 30);			//末尾收束
	depth_is_odd = g_find_depth % 2;
}

void goStart()
{
	scanf("%d", &myflag);
	myflag--;							//确定好子色
	enemyflag = myflag ? 0 : 1;
	initAI();								//初始化
	printf("OK\n");				//我准备好了
	fflush(stdout);
}
void goTurn()
{
	g_t_start = clock();											//计时
	struct Board tmp = g_board;								//棋盘tmp
	alphaBeta(&tmp, -INFINITY, INFINITY, g_find_depth, myflag);
	struct Command best_go = tmp.best_cmd;						//最好走这里						
	printf("%d", best_go.num_step);
	for (int i = 0; i < best_go.num_step; i++)
		printf(" %d,%d", best_go.x[i], best_go.y[i]);
	printf("\n");
	fflush(stdout);
	g_board = move(g_board, myflag, best_go);
}
void goPlace()
{
	struct Command commandtmp;
	scanf("%d", &commandtmp.num_step);
	for (int i = 0; i < commandtmp.num_step; i++)
		scanf("%d,%d", &commandtmp.x[i], &commandtmp.y[i]);
	g_board = move(g_board, enemyflag, commandtmp);
}
void goEnd()
{
	int score;
	scanf("%d", &score);
	exit(0);
}

void loop()							//即turn函数
{
	int turn = 0;
	char tmp[6];						//这些是正式的
	while (1)
	{
		scanf("%s", tmp);
		switch (orderCase(tmp))
		{
		case START:
			goStart();
			break;
		case TURN:
			goTurn();
			break;
		case PLACE:
			goPlace();
			break;
		case END:
			goEnd();
			break;
		}
		turn++;
		correctVariate(turn);
	}
}

int main()
{
	loop();
	return 0;
}

/**
 * 你的代码结束
 */