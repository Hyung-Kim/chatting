#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <time.h>


#define NICKSIZE 12
typedef SOCKET element;
typedef struct tagNode
{
	element data;
	char name[NICKSIZE];
	struct tagNode *next;
} NODE;
typedef struct List
{
	int numOfClient;
	NODE * head;
} LIST;
NODE *head, *tail, *working;
LIST * list = (List*)malloc(sizeof(List*));
void InitList()
{
	// 리스트초기화
	head = NULL;
	tail = NULL;
	working = NULL;
	list->numOfClient = 0;
}

int Search(LIST * list1, char *name)
{
	NODE * tmp;
	if (list1->head == NULL)
		return 1;
	else
	{
		tmp = list1->head;
		do
		{
			if (strcmp(tmp->name, name) == 0)
			{
				return 0;
			}
			tmp = tmp->next;
		} while (tmp);
	}
	return 1;
}
void Insert(element data1, char * name1)

{
	// 새로노드를하나만들어서값을대입
	working = (NODE *)malloc(sizeof(NODE));
	working->data = data1;
	
	strcpy_s(working->name ,name1);
	// 이것이꼬리임
	working->next = NULL;
	// 만약머리가비었으면이것이머리임
	if (list->head == NULL)
	{
		head = working;
		tail = working;
		list->head = head;
		return;
	}
	// 머리가아니라면마지막에노드를삽입하고
	tail->next = working;
	// 이것이꼬리임
	tail = working;
}

void Delete(element n)

{
	// 아무런데이타도없으면
	if (list->head == NULL)
		// 지울일도없다.
		return;
	// 일단머리를가져온다.
	working = list->head;
	NODE* node=NULL;
	// 같은값을찾아서삭제
	while (working)
	{
		if (working->data == n)  //같은값을 갖는다면
		{
			if (working == list->head)  //같은값을 갖는게 head라면
			{
				list->head = working->next;
				free(working);
			}
			else
			{
				node->next = working->next;
				free(working);
			}
			break;
		}
		node = working;
		working = working->next;

	}
}

