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
	// ����Ʈ�ʱ�ȭ
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
	// ���γ�带�ϳ�������������
	working = (NODE *)malloc(sizeof(NODE));
	working->data = data1;
	
	strcpy_s(working->name ,name1);
	// �̰��̲�����
	working->next = NULL;
	// ����Ӹ�����������̰��̸Ӹ���
	if (list->head == NULL)
	{
		head = working;
		tail = working;
		list->head = head;
		return;
	}
	// �Ӹ����ƴ϶�鸶��������带�����ϰ�
	tail->next = working;
	// �̰��̲�����
	tail = working;
}

void Delete(element n)

{
	// �ƹ�������Ÿ��������
	if (list->head == NULL)
		// �����ϵ�����.
		return;
	// �ϴܸӸ��������´�.
	working = list->head;
	NODE* node=NULL;
	// ��������ã�Ƽ�����
	while (working)
	{
		if (working->data == n)  //�������� ���´ٸ�
		{
			if (working == list->head)  //�������� ���°� head���
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

