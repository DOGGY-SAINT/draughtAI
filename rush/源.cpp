#include<stdio.h>					
#include <stdlib.h>									
#include <time.h>						
#include<string.h>

//������Ϣ
#define N_PIECES 12									//12����										
#define N_BOARD 8								//8*8����
#define N_STEP 13							//���13��
#define N_COMMAND 25				//һ���������25���߷���
#define EMPTY -1											//-1���ˣ�0��ͨ��1����
#define NORMAL 0										
#define SUPER 1
#define BLACK 0										//��0��1��������Ǻ�1��2����1�ͺ���
#define WHITE 1					
#define DOUBLE(a) (2*a)				//�ӱ�
#define START 1						//�����ж������õ�
#define TURN 2
#define PLACE 3
#define END 4
#define INFINITY 10000				//�����ab��
#define ANOTHERFLAG(x) (1-x)									//��һ��
#define TRUE 1
#define FALSE 0


//�����Ǳ�����ṹ��
int g_cut_time;									//�ض�ʱ��
int g_find_depth;							//������Ȳ�
int g_super_score;									//���ӵķ�
int g_normal_score;								//��ͨ�ӵķ�
typedef int FLAG;
typedef int BOOL;								//��¼�غ�,START����һ�ֹ�Ϊ0
const int direction[4][2] = { {-1,-1},{-1,1},{1,-1},{1,1} };	//����ͨ��0��1������ͨ��2��3
int myflag, enemyflag;
int g_t_start, g_t_end;
BOOL depth_is_odd;												//��¼Ҫ��������ż����

struct PieceMode			//��̬�洢
{
	int mode;			//-1��0��ͨ1��
	int x;			//��¼λ��
	int y;
};

struct Command
{
	int x[N_STEP];
	int y[N_STEP];
	int num_step;
};

struct AllCommand					//findEat��findMove�Ľ������Ҫ��ΪfindEat�ݹ���Ҫȫ�ִ�����������
{
	struct Command all_cmd[N_COMMAND];
}g_all_cmd;

struct Board						//������Ϣ
{
	char map[N_BOARD][N_BOARD];	//�����ϵĺ��Ӵ�0��11�����Ӵ�12��23
	struct PieceMode pieces[2][N_PIECES];	//0��1��
	struct Command best_cmd;			//�������Ž�
}g_board;			//��ǰ�����ϵľ���

/**
 * YOUR CODE BEGIN
 * ��Ĵ��뿪ʼ
 */

 //������һЩ����
BOOL isInside(int x, int y)				//�ڲ���������
{
	if (x >= 0 && x < N_BOARD)
		if (y >= 0 && y < N_BOARD)
			if ((x + y) % 2)
				return 1;
	return 0;
}

FLAG flagJudge(int num)				//�����������ã��ж����Ǻ��ǰ�
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

//��ֵ����
int countAll(struct Board tmp)					//��ֵ��Զ��ȫ���ҷ�ֵ���Է�ֵ
{
	int my_score = 0, enemy_score = 0;
	for (int i = 0; i < N_PIECES; i++)
	{
		my_score += countOne(tmp, tmp.pieces[myflag][i], myflag);
		enemy_score += countOne(tmp, tmp.pieces[enemyflag][i], enemyflag);
	}
	if (my_score == 0)						//������
		return -INFINITY;
	else if (enemy_score == 0)					//��Ӯ��
		return INFINITY;
	if (depth_is_odd)				//��ż��ֵ��һ��
		return -(my_score - enemy_score);
	else
		return my_score - enemy_score;
}

int mid(int x1, int x2)				//��ֵ
{
	if (abs(x1 - x2) == 1)
		return 0;
	else
		return (int)((x1 + x2) / 2);
}

struct Board move(struct Board in, FLAG flag, struct Command command)			//���Ӻ�����flag��ʾ��˭�µģ�0��1��
{
	int piecenum = in.map[command.x[0]][command.y[0]];
	in.pieces[flag][piecenum % N_PIECES].x = command.x[command.num_step - 1];
	in.pieces[flag][piecenum % N_PIECES].y = command.y[command.num_step - 1];
	in.map[command.x[0]][command.y[0]] = EMPTY;
	in.map[command.x[command.num_step - 1]][command.y[command.num_step - 1]] = piecenum;
	if (flag == 0 && command.x[command.num_step - 1] == 0)								//�ڣ���Ϊ0������
		in.pieces[flag][piecenum % N_PIECES].mode = 1;
	if (flag == 1 && command.x[command.num_step - 1] == 7)								//��,��Ϊ7������
		in.pieces[flag][piecenum % N_PIECES].mode = 1;										//������
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

//�ɳ�����
void findEat(struct Board temp, int x, int y, int flag, struct Command all_cmd)		//�������洢�ɳ�commmand��������g_all_cmd
{													//findEat��Ҫһ�����Ͻ�ȥ��command��һ��ȫ�ִ����command����
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
	if (all_cmd.num_step > g_all_cmd.all_cmd[0].num_step)					//�Ҳ���,����g_all_cmd��
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

//��������
void findMove(struct Board datatmp, int flag)		//˼·ͬ�ϣ��Ҳ�����g_all_cmd
{
	struct Command all_cmd;
	for (int k = 0; k < N_PIECES; k++)								//12����
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
				if (datatmp.map[togox][togoy] == EMPTY)					//Ϊ��
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
	int depth, FLAG flag)		//ab��֦
{
	g_t_end = clock();									//��ʱ��ʱ�䲻���Ͻ���
	if (g_t_end - g_t_start >= g_cut_time)
		return alpha;
	if (depth == 0)										//0���ֵ,��ż��һ��
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
	}																				//�����˿���·
	if (depth == g_find_depth && commandNum(g_all_cmd)==1)		//���ֻ��һ�������ߣ�ֱ���ߣ��Ͳ�����
	{
		current->best_cmd = g_all_cmd.all_cmd[0];
		return 0;
	}
	struct AllCommand local_all_cmd = g_all_cmd;		//�ø��ֲ�����������Ȼ���ϱ���
	for (int i = 0; i < N_COMMAND; i++)
	{
		int val;
		if (local_all_cmd.all_cmd[i].num_step >= 2)
		{
			struct Board next = move(*current, flag, local_all_cmd.all_cmd[i]);
			val = -alphaBeta(&next,
				-beta, -alpha,
				depth - 1, ANOTHERFLAG(flag));
			if (val >= beta)										//����û�и��õĽ���ˣ���
			{
				current->best_cmd = local_all_cmd.all_cmd[i];		//���Ĺ�ֵ��������Ͳ�����
				return beta;
			}
			if (val > alpha)												//���ĵ�ǰ��������Ҫ�Ľ��
			{
				alpha = val;
				current->best_cmd = local_all_cmd.all_cmd[i];
			}
			if (val == alpha)								//�е�ʱ��ȫ���ˣ���Ҫ����command��
				if (current->best_cmd.num_step < 2)
					current->best_cmd = local_all_cmd.all_cmd[i];
		}
		else
			return alpha;										//ȫ�����˾ͷ���
	}
}

int returnX(int a)			//����������Ӧ����������ʼ����ʱ����
{
	a /= 4;
	return a;
}

int returnY(int a)			//����������Ӧ����������ʼ����ʱ����
{
	int c = ((returnX(a) % 2) ? 2 * (a % 4) : 2 * (a % 4) + 1);
	return c;
}

int orderCase(char tmp[])			//����ʲô����Ѽ
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
 * ����������ﶨ�����Լ��Ľṹ��ͱ���
 */

 /**
  * ������������ʼ�����AI
  */
void initAI()											//��ʼ��
{
	memset(g_board.map, -1, sizeof(g_board.map));		//���̿տ���Ҳ	
	for (int i = 0; i < N_PIECES; i++)
	{
		g_board.pieces[1][i].mode = 0;							//0��1��
		g_board.pieces[1][i].x = returnX(i);
		g_board.pieces[1][i].y = returnY(i);
		g_board.map[returnX(i)][returnY(i)] = i + 12;
		g_board.pieces[0][i].mode = 0;
		g_board.pieces[0][i].x = returnX(i + 20);
		g_board.pieces[0][i].y = returnY(i + 20);
		g_board.map[returnX(i + 20)][returnY(i + 20)] = i;			//���̺�ģʽ����
	}
}

/**
 * �ֵ������ӡ�
 * ������0��ʾ�հף�1��ʾ���壬2��ʾ����
 * me��ʾ�������������(1��2)
 * ����Ҫ����һ���ṹ��Command������num_step����Ҫ�ƶ������Ӿ����ĸ�����������㡢�յ㣩��
 * x��y�ֱ��Ǹ��������ξ�����ÿ�����ӵĺᡢ������
 */

void correctV(int f_tmp, int c_tmp, int n_tmp, int s_tmp)
{
	g_find_depth = f_tmp;
	g_cut_time = c_tmp;
	g_normal_score = n_tmp;
	g_super_score = s_tmp;
}

void correctVariate(int turn)				//��������
{
	if (turn == 1 || turn == 2)											//��ͬ�׶β�ͬս��
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
		correctV(120 - turn + 1, 1890, 10, 30);			//ĩβ����
	depth_is_odd = g_find_depth % 2;
}

void goStart()
{
	scanf("%d", &myflag);
	myflag--;							//ȷ������ɫ
	enemyflag = myflag ? 0 : 1;
	initAI();								//��ʼ��
	printf("OK\n");				//��׼������
	fflush(stdout);
}
void goTurn()
{
	g_t_start = clock();											//��ʱ
	struct Board tmp = g_board;								//����tmp
	alphaBeta(&tmp, -INFINITY, INFINITY, g_find_depth, myflag);
	struct Command best_go = tmp.best_cmd;						//���������						
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

void loop()							//��turn����
{
	int turn = 0;
	char tmp[6];						//��Щ����ʽ��
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
 * ��Ĵ������
 */